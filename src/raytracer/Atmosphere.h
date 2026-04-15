#pragma once

#include "glm/glm.hpp"
#include <memory>

class WorldData;

/**
 * @brief Atmospheric scattering renderer
 *
 * Implements Rayleigh and Mie scattering for realistic sky rendering.
 * Based on the paper "Accurate Atmospheric Scattering" by Sean O'Neil
 * and the GPU Gems 2 implementation.
 */
class Atmosphere {
public:
    /**
     * @brief Construct atmosphere renderer
     */
    explicit Atmosphere(const WorldData& world);
    ~Atmosphere();

    /**
     * @brief Calculate sky color at given direction
     *
     * @param rayDir Direction to sample sky
     * @param time Time of day (0..1)
     * @return Sky color
     */
    glm::vec3 skyColor(const glm::vec3& rayDir, float time = 0.0f);

    /**
     * @brief Set sun direction
     */
    void setSunDirection(const glm::vec3& dir);

    /**
     * @brief Get current sun direction
     */
    glm::vec3 getSunDirection() const;

    /**
     * @brief Set atmospheric turbidity (1.0 = clear, 2.0 = hazy)
     */
    void setTurbidity(float t);

    /**
     * @brief Get turbidity
     */
    float getTurbidity() const;

    /**
     * @brief Calculate atmospheric extinction along a path
     */
    glm::vec3 atmosphericExtinction(const glm::vec3& start, const glm::vec3& end);

    /**
     * @brief Check if a direction points toward the sun
     */
    bool isSunDirection(const glm::vec3& dir, float threshold = 0.999f) const;

    /**
     * @brief Get sun disk color for direct sunlight
     */
    glm::vec3 sunDiskColor(const glm::vec3& rayDir) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Preetham sky model (alternative)
 */
class PreethamSky {
public:
    glm::vec3 skyColor(float sunY, float turbidity, float overcast);
};

/**
 * @brief Nishita sky model (physically based)
 */
class NishitaSky {
public:
    glm::vec3 skyColor(const glm::vec3& rayDir, const glm::vec3& sunDir,
                       float turbidity, float rayleigh, float mie);
};
