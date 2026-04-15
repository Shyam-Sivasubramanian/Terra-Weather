#pragma once

#include <cstdint>

/**
 * @brief Procedural noise generator
 */
class NoiseGen {
public:
    /**
     * @brief Create noise generator with seed
     */
    explicit NoiseGen(unsigned int seed);

    /**
     * @brief Perlin noise
     */
    float perlin(float x, float y, int octaves = 6, float persistence = 0.5f) const;

    /**
     * @brief Fractional Brownian Motion noise
     */
    float fbm(float x, float y, int octaves = 6, float lacunarity = 2.0f, float persistence = 0.5f) const;

    /**
     * @brief Ridged noise (mountains)
     */
    float ridged(float x, float y, int octaves = 6, float persistence = 0.5f, float ridgeOffset = 1.0f) const;

    /**
     * @brief Billow noise
     */
    float billow(float x, float y, int octaves = 6, float persistence = 0.5f) const;

    /**
     * @brief Voronoi noise (cells)
     */
    float voronoi(float x, float y, float jitter = 1.0f) const;

private:
    unsigned int seed;
};
