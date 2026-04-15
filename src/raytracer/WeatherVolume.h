#pragma once

#include "Ray.h"
#include "glm/glm.hpp"
#include <memory>

class WorldData;

/**
 * @brief Weather volume for rain and snow rendering
 *
 * Implements rain and snow as participating media using ray marching.
 * Particles are affected by wind and fall from cloud layer to ground.
 */
class WeatherVolume {
public:
    /**
     * @brief Construct weather volume renderer
     */
    explicit WeatherVolume(const WorldData& world);
    ~WeatherVolume();

    /**
     * @brief Render weather effects along a ray
     *
     * @param ray Ray through the scene
     * @param time Current time
     * @return Weather color contribution
     */
    glm::vec3 render(const Ray& ray, float time = 0.0f);

    /**
     * @brief Sample weather density at position
     */
    float density(const glm::vec3& pos, float time = 0.0f);

    /**
     * @brief Set weather intensity manually
     */
    void setWeatherIntensity(float rain, float snow);

    /**
     * @brief Set wind speed affecting particles
     */
    void setWindSpeed(float speed);

    /**
     * @brief Get current rain intensity
     */
    float getRainIntensity() const;

    /**
     * @brief Get current snow intensity
     */
    float getSnowIntensity() const;

    /**
     * @brief Update from world data
     */
    void update(const WorldData& world);

    /**
     * @brief Check if position is in weather volume
     */
    bool isInWeatherVolume(const glm::vec3& pos) const;

    /**
     * @brief Get rain/snow color
     */
    glm::vec3 weatherColor(const glm::vec3& pos) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Rain drop particle
 */
struct RainDrop {
    glm::vec3 position;
    float speed;
    float size;
};

/**
 * @brief Snow flake particle
 */
struct SnowFlake {
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float rotation;
};
