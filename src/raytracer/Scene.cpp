#include "Scene.h"
#include "WorldData.h"
#include "Materials.h"
#include "Material.h"
#include "TerrainHittable.h"
#include "BVH.h"

#include <cstdio>

Scene::~Scene() { clear(); }

void Scene::clear() {
    objects_.clear();
    bvh_.reset();
    for (Material* m : materials_) delete m;
    materials_.clear();
}

void Scene::build(const WorldData& world,
                  const char* grassPath,
                  const char* rockPath,
                  const char* snowPath) {
    clear();

    // --- Textures (optional) ---
    grassTex_.setFallback(glm::vec3(0.30f, 0.55f, 0.20f));
    rockTex_ .setFallback(glm::vec3(0.45f, 0.40f, 0.38f));
    snowTex_ .setFallback(glm::vec3(0.95f, 0.95f, 0.98f));
    if (grassPath && *grassPath) grassTex_.load(grassPath);
    if (rockPath  && *rockPath ) rockTex_ .load(rockPath);
    if (snowPath  && *snowPath ) snowTex_ .load(snowPath);

    // --- Materials ---
    const float snowY = world.snowLevel * world.heightScale;
    Material* terrainMat = Materials::makeTerrain(
        &grassTex_, &rockTex_, &snowTex_, snowY, world.heightScale);
    Material* waterMat = Materials::makeReflective(
        glm::vec3(0.05f, 0.12f, 0.18f), 0.02f);
    materials_.push_back(terrainMat);
    materials_.push_back(waterMat);

    // --- Terrain geometry chunks ---
    auto chunks = TerrainBuilder::build(world, terrainMat, waterMat, /*chunkSize*/ 16);
    objects_.insert(objects_.end(), chunks.begin(), chunks.end());

    // --- Compute world bounds by union of chunk AABBs ---
    bounds_ = AABB();
    for (const auto& h : objects_) {
        AABB b;
        if (h->boundingBox(b)) bounds_.expand(b);
    }

    // --- BVH root ---
    if (!objects_.empty()) {
        bvh_ = std::make_shared<BVHNode>(objects_, 0, objects_.size());
    }

    std::fprintf(stderr,
        "[Scene] built %zu chunks; bounds min=(%.1f,%.1f,%.1f) max=(%.1f,%.1f,%.1f)\n",
        objects_.size(),
        bounds_.min.x, bounds_.min.y, bounds_.min.z,
        bounds_.max.x, bounds_.max.y, bounds_.max.z);
}

bool Scene::hit(const Ray& r, float tmin, float tmax, HitRecord& rec) const {
    if (!bvh_) return false;
    return bvh_->hit(r, tmin, tmax, rec);
}
