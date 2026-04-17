#pragma once

#include "HitRecord.h"
#include "Material.h"
#include "WorldData.h"
#include "glm/glm.hpp"
#include <vector>
#include <memory>

/**
 * @brief Single terrain triangle for ray intersection
 */
struct TerrainTriangle {
    glm::vec3 v0, v1, v2;    // Triangle vertices
    int gridX, gridZ;         // Grid position
    float height;             // Approximate height
    std::shared_ptr<Material> material;  // Material for this triangle

    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const;
    AABB boundingBox(float time0, float time1) const;
};

/**
 * @brief Terrain mesh as hittable object
 *
 * Represents the procedural terrain as a grid mesh.
 * Each cell in the height map becomes two triangles.
 */
class TerrainHittable : public Hittable {
public:
    /**
     * @brief Construct terrain from world data
     */
    explicit TerrainHittable(const WorldData& world);

    /**
     * @brief Test ray intersection with terrain
     */
    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const override;

    /**
     * @brief Get bounding box for terrain
     */
    AABB boundingBox(float time0, float time1) const override;

    /**
     * @brief Get number of triangles
     */
    size_t triangleCount() const { return triangles.size(); }

private:
    const WorldData& worldData;
    int width, height;
    std::vector<TerrainTriangle> triangles;

    void buildMesh();
    std::shared_ptr<Material> getMaterial(int x, int z, float height) const;
    float perlinNoise(float x, float z) const;
    float calculateSlope(int x, int z) const;
    void buildQuadtree();
};

/**
 * @brief Water material with reflection/refraction
 */
class WaterMaterial : public Material {
public:
    WaterMaterial(const glm::vec3& color, float refractiveIndex, float waterLevel);

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override;

    glm::vec3 color;
    float refractiveIndex;
    float waterLevel;
};
