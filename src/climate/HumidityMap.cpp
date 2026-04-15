#include "HumidityMap.h"

HumidityMap::HumidityMap(WorldData& world) : world(world) {}

void HumidityMap::generate() {
    int width = world.width;
    int height = world.height;

    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            float height = world.get(world.heightMap, x, z);

            // Base humidity from height (lower = more humid, near water)
            float baseHumidity = 1.0f - height;

            // Increase humidity near water
            if (height < world.seaLevel + 0.05f) {
                baseHumidity = 1.0f;
            }

            // Mountains can be drier at peaks
            if (height > world.snowLevel - 0.1f) {
                baseHumidity *= 0.7f;
            }

            // Add some variation
            float variation = simpleNoise(x * 0.1f, z * 0.1f) * 0.2f;
            float humidity = baseHumidity + variation;

            // Clamp
            humidity = std::clamp(humidity, 0.0f, 1.0f);

            world.humidityMap[z * width + x] = humidity;
        }
    }

    // Apply rain shadow effect
    applyRainShadow();
}

void HumidityMap::applyRainShadow() {
    // Rain shadow: leeward side of mountains is drier
    int width = world.width;
    int height = world.height;

    for (int pass = 0; pass < 3; pass++) {
        for (int z = 1; z < height - 1; z++) {
            for (int x = 1; x < width - 1; x++) {
                float h = world.get(world.heightMap, x, z);

                // Check if uphill in wind direction (wind from +X)
                float uphillH = world.get(world.heightMap, x + 1, z);

                if (uphillH > h + 0.05f) {
                    // Reduce humidity on leeward side
                    world.humidityMap[z * width + x] *= 0.95f;
                }
            }
        }
    }
}

float HumidityMap::simpleNoise(float x, float y) const {
    // Very simple noise
    float v = sin(x * 1.2f) * cos(y * 0.8f);
    v += sin(x * 2.3f + y * 1.7f) * 0.5f;
    v += cos(x * 0.9f - y * 2.1f) * 0.3f;
    return v / 1.8f;
}
