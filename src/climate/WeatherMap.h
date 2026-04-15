#pragma once

#include "WorldData.h"

/**
 * @brief Weather map generation
 */
class WeatherMap {
public:
    /**
     * @brief Create weather map generator
     */
    explicit WeatherMap(WorldData& world);

    /**
     * @brief Generate weather map based on humidity and temperature
     */
    void generate();

    /**
     * @brief Update weather based on time
     */
    void update(float time);

private:
    WorldData& world;
    float simpleNoise(float x, float y) const;
};
