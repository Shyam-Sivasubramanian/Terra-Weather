#include "Scene.h"
#include "TerrainHittable.h"
#include "BVH.h"
#include <algorithm>

/**
 * @brief Construct scene from world data
 *
 * Builds all hittable objects from the shared WorldData.
 * Currently includes terrain; extensible for other objects.
 */
Scene::Scene(const WorldData& world)
    : worldData(world), needsRebuild(true) {
    // Create terrain hittable
    terrain = std::make_shared<TerrainHittable>(worldData);

    // Add terrain to scene objects
    objects.push_back(terrain);
}

/**
 * @brief Rebuild acceleration structures if needed
 */
void Scene::ensureBuilt() {
    if (needsRebuild) {
        rebuildBVH();
        needsRebuild = false;
    }
}

/**
 * @brief Rebuild BVH for terrain acceleration
 */
void Scene::rebuildBVH() {
    // For now, terrain handles its own grid-based acceleration
    // BVH can be added later if needed for more complex scenes
    bvh = std::make_unique<BVH>();

    // Extract triangles from terrain and build BVH
    std::vector<TrianglePrimitive> triangles;
    // This would extract triangles from terrain for BVH construction
    // For now, terrain uses its built-in grid acceleration

    // bvh->build(triangles);
}

/**
 * @brief Update scene when world data changes
 */
void Scene::update() {
    // Rebuild terrain if height map changed
    terrain = std::make_shared<TerrainHittable>(worldData);
    objects.clear();
    objects.push_back(terrain);
    needsRebuild = true;
}

/**
 * @brief Add a hittable object to the scene
 */
void Scene::add(std::shared_ptr<Hittable> object) {
    objects.push_back(object);
    needsRebuild = true;
}

/**
 * @brief Add a hittable object with its BVH node
 */
void Scene::addBVH(std::shared_ptr<BVH> object) {
    bvhObjects.push_back(object);
    needsRebuild = true;
}

/**
 * @brief Trace ray through entire scene
 *
 * Tests ray intersection against all scene objects.
 * Returns the closest valid hit.
 */
bool Scene::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    bool hitAnything = false;
    float closestT = tmax;
    HitRecord tempRec;

    // Test against terrain
    if (terrain && terrain->hit(ray, tmin, closestT, tempRec)) {
        closestT = tempRec.t;
        rec = tempRec;
        hitAnything = true;
    }

    // Test against general hittable list
    for (const auto& obj : objects) {
        if (obj->hit(ray, tmin, closestT, tempRec)) {
            closestT = tempRec.t;
            rec = tempRec;
            hitAnything = true;
        }
    }

    // Test against BVH objects
    for (const auto& obj : bvhObjects) {
        if (obj->hit(ray, tmin, closestT, tempRec)) {
            closestT = tempRec.t;
            rec = tempRec;
            hitAnything = true;
        }
    }

    return hitAnything;
}

/**
 * @brief Get bounding box of entire scene
 */
AABB Scene::boundingBox(float time0, float time1) const {
    if (objects.empty()) {
        return AABB();
    }

    AABB box = objects[0]->boundingBox(time0, time1);
    for (size_t i = 1; i < objects.size(); i++) {
        box.expand(objects[i]->boundingBox(time0, time1));
    }
    return box;
}

/**
 * @brief Get scene bounding box (world space)
 */
AABB Scene::getWorldBounds() const {
    AABB box;
    box.min = glm::vec3(0.0f, 0.0f, 0.0f);
    box.max = glm::vec3(1.0f, 1.0f, 1.0f);
    return box;
}

/**
 * @brief Get reference to world data
 */
const WorldData& Scene::getWorldData() const {
    return worldData;
}

/**
 * @brief Get terrain hittable
 */
std::shared_ptr<TerrainHittable> Scene::getTerrain() const {
    return terrain;
}

/**
 * @brief Get world position from camera ray
 *
 * Used for terrain shading based on hit position.
 */
glm::vec3 Scene::worldPosition(const HitRecord& hit) const {
    return hit.point;
}

/**
 * @brief Check if a ray hits terrain
 */
bool Scene::hitsTerrain(const Ray& ray) const {
    HitRecord rec;
    return terrain && terrain->hit(ray, 0.001f, 1000.0f, rec);
}

/**
 * @brief Get terrain height at world position
 */
float Scene::getTerrainHeight(float x, float z) const {
    // Convert world coords to grid coords
    int gx = static_cast<int>(x * (worldData.width - 1));
    int gz = static_cast<int>(z * (worldData.height - 1));
    return worldData.getHeightWorld(x, z);
}

/**
 * @brief Set directional light (sun) parameters
 */
void Scene::setSunDirection(const glm::vec3& dir) {
    sunDirection = glm::normalize(dir);
}

void Scene::setSunColor(const glm::vec3& color) {
    sunColor = color;
}

void Scene::setSunIntensity(float intensity) {
    sunIntensity = intensity;
}

/**
 * @brief Get sun parameters
 */
glm::vec3 Scene::getSunDirection() const { return sunDirection; }
glm::vec3 Scene::getSunColor() const { return sunColor; }
float Scene::getSunIntensity() const { return sunIntensity; }

/**
 * @brief Get ambient light color
 */
glm::vec3 Scene::getAmbientLight() const {
    return ambientLight;
}

void Scene::setAmbientLight(const glm::vec3& color) {
    ambientLight = color;
}

/**
 * @brief Sky dome hittable for atmospheric background
 */
class SkyDome : public Hittable {
public:
    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const override {
        // Sky dome always hits, at infinity
        // This is a placeholder - actual sky color computed in trace()
        rec.t = std::numeric_limits<float>::infinity();
        rec.point = ray.at(rec.t);
        rec.normal = glm::normalize(ray.origin - rec.point);
        return true;
    }

    AABB boundingBox(float time0, float time1) const override {
        return AABB(
            glm::vec3(-1e10f),
            glm::vec3(1e10f)
        );
    }
};

/**
 * @brief Add sky dome to scene
 */
void Scene::addSkyDome() {
    skyDome = std::make_shared<SkyDome>();
}

/**
 * @brief Check for sky hit (no terrain intersection)
 */
bool Scene::isSky(const Ray& ray) const {
    // If no terrain hit, ray hits sky
    return !hitsTerrain(ray);
}
