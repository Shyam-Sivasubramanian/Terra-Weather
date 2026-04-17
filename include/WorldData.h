#include <cmath>
#pragma once

#include <vector>
#include <algorithm>

/**
 * @brief Shared data structure containing all terrain and climate simulation data
 *
 * This struct is the central data hub that all subsystems (terrain generation,
 * climate simulation, ray tracing) read from. Shyam's terrain code writes to
 * heightMap, seaLevel, snowLevel. Janavi's climate code writes to humidityMap,
 * temperatureMap, windU, windV, weatherMap, cloudDensity.
 */
struct WorldData {
    // World dimensions
    int width = 512;
    int height = 512;

    // Random seed for reproducible generation
    unsigned int seed = 42;

    // Terrain data (Shyam writes)
    std::vector<float> heightMap;  // [0, 1] normalized elevation
    float seaLevel = 0.35f;        // Water starts at this height
    float snowLevel = 0.85f;       // Snow starts at this height

    // Climate data (Janavi writes)
    std::vector<float> humidityMap;     // [0, 1] humidity percentage
    std::vector<float> temperatureMap;  // [0, 1] temperature (can be remapped to actual temp)
    std::vector<float> windU;          // Wind velocity U component
    std::vector<float> windV;          // Wind velocity V component
    std::vector<float> weatherMap;      // 0=clear, 1=rain, 2=snow
    std::vector<float> cloudDensity;    // [0, 1] volumetric cloud density
    float getCloudDensity(float x, float y, float z) const { return cloudDensity.empty() ? 0.0f : cloudDensity[0]; }

    /**
     * @brief Safely sample a map with boundary clamping
     * @param m The map to sample
     * @param x X coordinate (will be clamped to [0, width-1])
     * @param z Z coordinate (will be clamped to [0, height-1])
     * @return The sampled value at the clamped coordinates
     */
    float get(const std::vector<float>& m, int x, int z) const {
        int cx = std::clamp(x, 0, width - 1);
        int cz = std::clamp(z, 0, height - 1);
        return m[cz * width + cx];
    }

    /**
     * @brief Bilinear interpolation sample
     */
    float getBilinear(const std::vector<float>& m, float x, float z) const {
        int x0 = static_cast<int>(std::floor(x));
        int z0 = static_cast<int>(std::floor(z));
        float fx = x - x0;
        float fz = z - z0;

        float v00 = get(m, x0, z0);
        float v10 = get(m, x0 + 1, z0);
        float v01 = get(m, x0, z0 + 1);
        float v11 = get(m, x0 + 1, z0 + 1);

        float v0 = v00 * (1 - fx) + v10 * fx;
        float v1 = v01 * (1 - fx) + v11 * fx;

        return v0 * (1 - fz) + v1 * fz;
    }

    /**
     * @brief Get height at world position (x, z) in world units
     */
    float getHeightWorld(float wx, float wz) const {
        float x = wx * (width - 1);
        float z = wz * (height - 1);
        return getBilinear(heightMap, x, z);
    }

    /**
     * @brief Check if a point is below sea level
     */
    bool isUnderwater(float height) const {
        return height < seaLevel;
    }

    /**
     * @brief Check if a point has snow coverage
     */
    bool isSnowCovered(float height) const {
        return height > snowLevel;
    }

    /**
     * @brief Get terrain type at a point based on height and climate
     */
    enum class TerrainType {
        Water,
        Sand,
        Grass,
        Rock,
        Snow
    };

    TerrainType getTerrainType(int x, int z) const {
        float h = get(heightMap, x, z);

        if (h < seaLevel) return TerrainType::Water;
        if (h < seaLevel + 0.02f) return TerrainType::Sand;
        if (h < snowLevel - 0.1f) {
            float humidity = get(humidityMap, x, z);
            float temp = get(temperatureMap, x, z);
            // High humidity and moderate temp suggests forest/grass
            if (humidity > 0.5f && temp > 0.4f) return TerrainType::Grass;
            return TerrainType::Grass;
        }
        if (h < snowLevel) return TerrainType::Rock;
        return TerrainType::Snow;
    }

    /**
     * @brief Get temperature at grid position
     */
    float getTemperature(int x, int z) const {
        return get(temperatureMap, x, z);
    }

    /**
     * @brief Set temperature at grid position
     */
    void setTemperature(int x, int z, float temp) {
        x = std::clamp(x, 0, width - 1);
        z = std::clamp(z, 0, height - 1);
        temperatureMap[z * width + x] = temp;
    }
};
