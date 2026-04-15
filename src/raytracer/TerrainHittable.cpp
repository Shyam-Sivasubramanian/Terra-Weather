#include "TerrainHittable.h"
#include <algorithm>
#include <cmath>

/**
 * @brief Triangle-ray intersection using Möller-Trumbore algorithm
 *
 * This is the fastest triangle intersection algorithm for ray tracing.
 * It tests intersection directly against the triangle vertices.
 */
bool TerrainTriangle::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);

    // Ray is parallel to triangle (culling case)
    if (fabs(a) < 0.0001f) return false;

    float f = 1.0f / a;
    glm::vec3 s = ray.origin - v0;
    float u = f * dot(s, h);

    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);

    if (v < 0.0f || u + v > 1.0f) return false;

    float t = f * dot(edge2, q);

    if (t > tmin && t < tmax) {
        rec.t = t;
        rec.point = ray.origin + t * ray.direction;
        rec.normal = glm::normalize(cross(edge1, edge2));
        if (glm::dot(rec.normal, ray.direction) > 0) {
            rec.normal = -rec.normal;  // Ensure normal faces ray
        }

        // Texture coordinates based on barycentric
        rec.uv = glm::vec2(u, v);

        return true;
    }

    return false;
}

/**
 * @brief Get bounding box for single triangle
 */
AABB TerrainTriangle::boundingBox(float time0, float time1) const {
    AABB box;
    box.expand(v0);
    box.expand(v1);
    box.expand(v2);
    // Add small epsilon to avoid edge cases
    box.min -= glm::vec3(0.0001f);
    box.max += glm::vec3(0.0001f);
    return box;
}

/**
 * @brief Constructor for terrain mesh hittable
 *
 * Builds a grid mesh from height data. Each cell becomes two triangles.
 */
TerrainHittable::TerrainHittable(const WorldData& world)
    : worldData(world), width(world.width), height(world.height) {
    buildMesh();
}

/**
 * @brief Build triangle mesh from height map data
 */
void TerrainHittable::buildMesh() {
    triangles.clear();
    triangles.reserve((width - 1) * (height - 1) * 2);

    // World scale
    float worldWidth = 1.0f;
    float worldHeight = 1.0f;
    float cellWidth = worldWidth / (width - 1);
    float cellHeight = worldHeight / (height - 1);

    for (int z = 0; z < height - 1; z++) {
        for (int x = 0; x < width - 1; x++) {
            // Get heights at four corners
            float h00 = worldData.get(worldData.heightMap, x, z);
            float h10 = worldData.get(worldData.heightMap, x + 1, z);
            float h01 = worldData.get(worldData.heightMap, x, z + 1);
            float h11 = worldData.get(worldData.heightMap, x + 1, z + 1);

            // World coordinates
            float wx0 = x * cellWidth;
            float wz0 = z * cellHeight;
            float wx1 = (x + 1) * cellWidth;
            float wz1 = (z + 1) * cellHeight;

            // Triangle 1: (0,0), (1,0), (0,1)
            {
                TerrainTriangle tri;
                tri.v0 = glm::vec3(wx0, h00, wz0);
                tri.v1 = glm::vec3(wx1, h10, wz0);
                tri.v2 = glm::vec3(wx0, h01, wz1);
                tri.gridX = x;
                tri.gridZ = z;
                tri.height = h00;
                tri.material = getMaterial(x, z, h00);
                triangles.push_back(tri);
            }

            // Triangle 2: (1,0), (1,1), (0,1)
            {
                TerrainTriangle tri;
                tri.v0 = glm::vec3(wx1, h10, wz0);
                tri.v1 = glm::vec3(wx1, h11, wz1);
                tri.v2 = glm::vec3(wx0, h01, wz1);
                tri.gridX = x;
                tri.gridZ = z;
                tri.height = h00;
                tri.material = getMaterial(x, z, h00);
                triangles.push_back(tri);
            }
        }
    }
}

/**
 * @brief Determine material based on terrain properties
 */
std::shared_ptr<Material> TerrainHittable::getMaterial(int x, int z, float height) const {
    if (height < worldData.seaLevel) {
        // Water material
        return std::make_shared<WaterMaterial>(
            glm::vec3(0.05f, 0.15f, 0.3f),  // Deep water color
            1.33f,                            // Water refractive index
            worldData.seaLevel                // Water level
        );
    }

    if (height > worldData.snowLevel) {
        // Snow - bright white with slight blue tint
        float noise = perlinNoise(x * 0.1f, z * 0.1f);
        return std::make_shared<Lambertian>(glm::vec3(0.95f, 0.97f, 1.0f) + noise * 0.05f);
    }

    // Calculate slope for rock detection
    float slope = calculateSlope(x, z);

    if (slope > 0.7f) {
        // Steep slope = rock
        return std::make_shared<Lambertian>(glm::vec3(0.4f, 0.35f, 0.3f));
    }

    // Grass with color variation based on humidity and temperature
    float humidity = worldData.get(worldData.humidityMap, x, z);
    float temp = worldData.get(worldData.temperatureMap, x, z);

    // Dry grass is more yellow, humid grass is greener
    glm::vec3 grassColor = glm::mix(
        glm::vec3(0.7f, 0.6f, 0.3f),  // Dry/yellow grass
        glm::vec3(0.2f, 0.5f, 0.15f), // Wet/green grass
        humidity
    );

    // Adjust for temperature (cooler = more blue/gray)
    grassColor = glm::mix(grassColor, glm::vec3(0.4f, 0.45f, 0.35f), 1.0f - temp);

    return std::make_shared<Lambertian>(grassColor);
}

/**
 * @brief Simple Perlin-like noise for terrain variation
 */
float TerrainHittable::perlinNoise(float x, float z) const {
    // Simplified noise - in production, use actual Perlin/Simplex noise
    return 0.5f * sin(x * 0.5f) * cos(z * 0.5f) + 0.5f;
}

/**
 * @brief Calculate slope at grid position (0 = flat, 1 = vertical)
 */
float TerrainHittable::calculateSlope(int x, int z) const {
    float h00 = worldData.get(worldData.heightMap, x, z);
    float h10 = worldData.get(worldData.heightMap, x + 1, z);
    float h01 = worldData.get(worldData.heightMap, x, z + 1);

    float dx = h10 - h00;
    float dz = h01 - h00;

    // Gradient magnitude
    return sqrt(dx * dx + dz * dz) * 10.0f;
}

/**
 * @brief Hit test against terrain mesh
 *
 * Uses grid-based acceleration: first determine which grid cells the ray
 * passes through, then test only those triangles.
 */
bool TerrainHittable::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    bool hitAnything = false;
    float closestT = tmax;

    // Grid-based acceleration
    float worldWidth = 1.0f;
    float worldHeight = 1.0f;
    float cellWidth = worldWidth / (width - 1);
    float cellHeight = worldHeight / (height - 1);

    // Ray-box intersection first
    AABB box = boundingBox(0.0f, 1.0f);
    if (!box.hit(ray, tmin, tmax)) return false;

    // Walk through grid cells the ray passes through
    glm::vec3 invDir(1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z);

    // Start cell
    int x = static_cast<int>(ray.origin.x / cellWidth);
    int z = static_cast<int>(ray.origin.z / cellHeight);

    // Step direction
    int stepX = (ray.direction.x > 0) ? 1 : -1;
    int stepZ = (ray.direction.z > 0) ? 1 : -1;

    // Distance to next cell boundary
    float tMaxX = ((ray.direction.x > 0 ? x + 1 : x) * cellWidth - ray.origin.x) * invDir.x;
    float tMaxZ = ((ray.direction.z > 0 ? z + 1 : z) * cellHeight - ray.origin.z) * invDir.z;

    // Cell to next boundary
    float tDeltaX = cellWidth * invDir.x * stepX;
    float tDeltaZ = cellHeight * invDir.z * stepZ;

    // Clamp starting cell
    x = std::clamp(x, 0, width - 2);
    z = std::clamp(z, 0, height - 2);

    // Walk through cells
    while (closestT > 0 && x >= 0 && x < width - 1 && z >= 0 && z < height - 1) {
        // Test triangles in this cell
        int triIndex = (z * (width - 1) + x) * 2;

        // Triangle 1
        {
            HitRecord tempRec;
            if (triangles[triIndex].hit(ray, tmin, closestT, tempRec)) {
                if (tempRec.t < closestT) {
                    closestT = tempRec.t;
                    rec = tempRec;
                    hitAnything = true;
                }
            }
        }

        // Triangle 2
        {
            HitRecord tempRec;
            if (triangles[triIndex + 1].hit(ray, tmin, closestT, tempRec)) {
                if (tempRec.t < closestT) {
                    closestT = tempRec.t;
                    rec = tempRec;
                    hitAnything = true;
                }
            }
        }

        // Move to next cell
        if (tMaxX < tMaxZ) {
            tMaxX += tDeltaX;
            x += stepX;
        } else {
            tMaxZ += tDeltaZ;
            z += stepZ;
        }
    }

    return hitAnything;
}

/**
 * @brief Get bounding box for entire terrain
 */
AABB TerrainHittable::boundingBox(float time0, float time1) const {
    AABB box;
    box.min = glm::vec3(0.0f, 0.0f, 0.0f);
    box.max = glm::vec3(1.0f, 1.0f, 1.0f);
    return box;
}

/**
 * @brief Water material implementation
 */
WaterMaterial::WaterMaterial(const glm::vec3& color, float refractiveIndex, float waterLevel)
    : color(color), refractiveIndex(refractiveIndex), waterLevel(waterLevel) {}

bool WaterMaterial::scatter(const Ray& rayIn, const HitRecord& rec,
                            glm::vec3& attenuation, Ray& scattered) const {
    // Calculate Fresnel reflectance
    float cosine = glm::dot(-rayIn.direction, rec.normal);
    float reflectProb = schlick(cosine, refractiveIndex);

    // Reflection
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    if (dis(gen) < reflectProb) {
        // Pure reflection
        glm::vec3 reflected = reflect(rayIn.direction, rec.normal);
        scattered = Ray(rec.point, reflected);
        attenuation = glm::vec3(0.9f);
    } else {
        // Refraction with absorption
        glm::vec3 refracted;
        if (refract(rayIn.direction, rec.normal, 1.0f / refractiveIndex, refracted)) {
            scattered = Ray(rec.point, refracted);
        } else {
            scattered = Ray(rec.point, reflect(rayIn.direction, rec.normal));
        }

        // Water absorption (deeper = darker, bluer)
        float depth = waterLevel - rec.point.y;
        float absorption = exp(-depth * 2.0f);
        attenuation = color * absorption + glm::vec3(0.7f) * (1.0f - absorption);
    }

    return true;
}

/**
 * @brief Create quad tree for LOD terrain (optional optimization)
 */
void TerrainHittable::buildQuadtree() {
    // Optional: Build quadtree for faster ray-terrain intersection
    // This can significantly speed up rendering for large terrains
    // Implementation would recursively subdivide the terrain
    // and only traverse nodes that the ray intersects
}
