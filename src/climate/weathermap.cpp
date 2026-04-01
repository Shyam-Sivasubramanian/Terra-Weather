#pragma once

#include <vector>
#include <algorithm>

struct WorldData {
    int width = 0;
    int height = 0;
    unsigned int seed = 42;

    std::vector<float> heightMap;
    std::vector<float> humidityMap;
    std::vector<float> temperatureMap;
    std::vector<float> windU;
    std::vector<float> windV;
    std::vector<float> weatherMap;
    std::vector<float> cloudDensity;

    float seaLevel = 0.35f;
    float snowLevel = 0.75f;

    float get(const std::vector<float>& m, int x, int z) const {
        if (width <= 0 || height <= 0 || m.empty()) return 0.0f;
        x = std::clamp(x, 0, width - 1);
        z = std::clamp(z, 0, height - 1);
        return m[z * width + x];
    }
};