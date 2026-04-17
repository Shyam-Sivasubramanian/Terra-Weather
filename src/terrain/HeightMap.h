#pragma once

#include "WorldData.h"
#include "NoiseGen.h"

class NoiseGen;

/**
 * @brief Height map generation and processing
 */
class HeightMap {
public:
    WorldData& world;
public:
    WorldData& world;
public:
    /**
     * @brief Create height map generator
     */
    explicit HeightMap(WorldData& world);

    /**
     * @brief Generate height map from noise
     */
    void generate(const NoiseGen& noise);

    /**
     * @brief Apply erosion simulation
     */
    void applyErosion(int iterations = 100, float erosionStrength = 0.1f);

    /**
     * @brief Normalize heights to [0, 1]
     */
    void normalize();

    /**
     * @brief Get height at position
     */
    float getHeight(int x, int z) const {
        return world.get(world.heightMap, x, z);
    }
};
