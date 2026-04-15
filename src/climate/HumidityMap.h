#pragma once

#include "WorldData.h"

/**
 * @brief Humidity map generation
 */
class HumidityMap {
public:
    /**
     * @brief Create humidity map generator
     */
    explicit HumidityMap(WorldData& world);

    /**
     * @brief Generate humidity map based on terrain and climate
     */
    void generate();

    /**
     * @brief Apply rain shadow effect
     */
    void applyRainShadow();

    /**
     * @brief Apply blur for smoothing
     */
    void applyBlur(int radius = 2);

private:
    WorldData& world;
    float simpleNoise(float x, float y) const;
};
