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
