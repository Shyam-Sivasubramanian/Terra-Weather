#pragma once

#include "Ray.h"
#include "glm/glm.hpp"
#include <memory>

class WorldData;

/**
 * @brief Volumetric cloud rendering
 *
 * Implements physically-based volumetric cloud rendering using ray marching.
 * Clouds are rendered as participating media with absorption and scattering.
 */
class VolumetricCloud {
public:
    /**
     * @brief Construct volumetric cloud renderer
     */
    explicit VolumetricCloud(const WorldData& world);
    ~VolumetricCloud();

    /**
     * @brief Render clouds along a ray
     *
     * @param ray Ray through the scene
     * @param time Time for animated clouds
     * @return Cloud color contribution
     */
    glm::vec3 render(const Ray& ray, float time = 0.0f);

    /**
     * @brief Sample cloud density at position
     */
    float density(const glm::vec3& pos) const;

    /**
     * @brief Set cloud layer height bounds
     */
    void setCloudHeight(float base, float top);

    /**
     * @brief Set overall density multiplier
     */
    void setDensityScale(float scale);

    /**
     * @brief Set light absorption coefficient
     */
    void setAbsorption(float absorption);

    /**
     * @brief Set ray march quality
     */
    void setMarchSteps(int volumeSteps, int lightSteps);

    /**
     * @brief Check if position is inside cloud volume
     */
    bool isInCloudVolume(const glm::vec3& pos) const;

    /**
     * @brief Get cloud color at position
     */
    glm::vec3 cloudColor(const glm::vec3& pos, const glm::vec3& viewDir,
                        const glm::vec3& lightDir) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Cumulus cloud shape
 */
class CumulusCloud {
public:
    float density(const glm::vec3& pos) const;

private:
    float baseRadius = 100.0f;
    float topRadius = 200.0f;
    float height = 150.0f;
};

/**
 * @brief Stratus cloud layer
 */
class StratusCloud {
public:
    float density(const glm::vec3& pos) const;

private:
    float thickness = 50.0f;
    float coverage = 0.5f;
};
