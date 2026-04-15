#pragma once

#include "HitRecord.h"
#include "BVH.h"
#include "TerrainHittable.h"
#include "WorldData.h"
#include "glm/glm.hpp"
#include <memory>
#include <vector>

/**
 * @brief Scene containing all hittable objects for ray tracing
 *
 * Manages the scene graph, acceleration structures, and lighting.
 * All subsystems (terrain, atmosphere, climate) are assembled here.
 */
class Scene {
public:
    /**
     * @brief Construct scene from world data
     */
    explicit Scene(const WorldData& world);

    /**
     * @brief Ensure acceleration structures are built
     */
    void ensureBuilt();

    /**
     * @brief Rebuild scene (when world data changes)
     */
    void update();

    /**
     * @brief Add hittable object to scene
     */
    void add(std::shared_ptr<Hittable> object);

    /**
     * @brief Add BVH-accelerated object to scene
     */
    void addBVH(std::shared_ptr<BVH> object);

    /**
     * @brief Trace ray through scene
     *
     * Tests intersection against all scene objects.
     * @return true if any hit occurred
     */
    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const;

    /**
     * @brief Get scene bounding box
     */
    AABB boundingBox(float time0, float time1) const;

    /**
     * @brief Get world bounds
     */
    AABB getWorldBounds() const;

    /**
     * @brief Get world data reference
     */
    const WorldData& getWorldData() const;

    /**
     * @brief Get terrain hittable
     */
    std::shared_ptr<TerrainHittable> getTerrain() const;

    /**
     * @brief Get world position from hit record
     */
    glm::vec3 worldPosition(const HitRecord& hit) const;

    /**
     * @brief Check if ray hits terrain
     */
    bool hitsTerrain(const Ray& ray) const;

    /**
     * @brief Get terrain height at world position
     */
    float getTerrainHeight(float x, float z) const;

    // Sun/Lighting parameters
    void setSunDirection(const glm::vec3& dir);
    void setSunColor(const glm::vec3& color);
    void setSunIntensity(float intensity);
    glm::vec3 getSunDirection() const;
    glm::vec3 getSunColor() const;
    float getSunIntensity() const;

    // Ambient lighting
    glm::vec3 getAmbientLight() const;
    void setAmbientLight(const glm::vec3& color);

    // Sky dome
    void addSkyDome();
    bool isSky(const Ray& ray) const;

private:
    const WorldData& worldData;

    // Hittable objects
    std::vector<std::shared_ptr<Hittable>> objects;
    std::vector<std::shared_ptr<BVH>> bvhObjects;
    std::shared_ptr<TerrainHittable> terrain;
    std::shared_ptr<Hittable> skyDome;

    // Acceleration structures
    std::unique_ptr<BVH> bvh;
    bool needsRebuild;

    // Lighting
    glm::vec3 sunDirection = glm::normalize(glm::vec3(0.5f, 0.8f, 0.3f));
    glm::vec3 sunColor = glm::vec3(1.0f, 0.98f, 0.95f);
    float sunIntensity = 1.0f;
    glm::vec3 ambientLight = glm::vec3(0.1f, 0.12f, 0.15f);

    void rebuildBVH();
};
