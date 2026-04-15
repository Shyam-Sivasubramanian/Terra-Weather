#include "Precipitation.h"

Precipitation::Precipitation(WorldData& world) : world(world) {}

void Precipitation::update(float deltaTime) {
    // Simple precipitation accumulation
    int width = world.width;
    int height = world.height;

    // Accumulation rate
    float accumulationRate = 0.001f;

    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            float weather = world.get(world.weatherMap, x, z);
            float humidity = world.get(world.humidityMap, x, z);

            // Accumulate precipitation when raining
            if (weather > 0.5f) {
                // Simple accumulation
                float newWeather = weather + accumulationRate * deltaTime * humidity;
                world.weatherMap[z * width + x] = std::clamp(newWeather, 0.0f, 2.0f);
            } else {
                // Decay back to base
                float baseWeather = 0.0f;
                float newWeather = weather + (baseWeather - weather) * deltaTime * 0.1f;
                world.weatherMap[z * width + x] = newWeather;
            }
        }
    }
}

void Precipitation::applyEvaporation(float rate) {
    for (auto& w : world.weatherMap) {
        w = std::max(0.0f, w - rate);
    }
}
