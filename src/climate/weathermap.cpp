<<<<<<< HEAD
#include "WeatherMap.h"

WeatherMap::WeatherMap(WorldData& world) : world(world) {}

void WeatherMap::generate() {
    int width = world.width;
    int height = world.height;

    // Weather states: 0 = clear, 1 = rain, 2 = snow
    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            float h = world.get(world.heightMap, x, z);
            float humidity = world.get(world.humidityMap, x, z);
            float temp = world.getTemperature(x, z);

            // Temperature based on height
            // temp = temperatureMap already accounts for latitude and altitude

            float weather = 0.0f;  // Clear

            // Determine weather based on conditions
            if (h < world.snowLevel && humidity > 0.6f && temp < 0.4f) {
                // Snow conditions
                weather = 2.0f;
            } else if (h < world.snowLevel && humidity > 0.7f && temp > 0.4f) {
                // Rain conditions
                weather = 1.0f;
            }

            // Add temporal variation
            float variation = simpleNoise(x * 0.05f, z * 0.05f) * 0.3f;
            weather = std::clamp(weather + variation, 0.0f, 2.0f);

            world.weatherMap[z * width + x] = weather;
        }
    }
}

float WeatherMap::simpleNoise(float x, float y) const {
    return sin(x * 1.1f) * cos(y * 0.7f) * 0.5f;
}
=======
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
>>>>>>> fa2aca87a26a39699bf9f62c31cd09d43d385afd
