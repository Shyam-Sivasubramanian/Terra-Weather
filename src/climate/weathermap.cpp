#include "Climate.h"
#include "WorldData.h"

#include <algorithm>
#include <vector>
#include <cmath>

namespace WeatherMap {

void build(WorldData& world) {
    const int W = world.width;
    const int H = world.height;
    if (W <= 0 || H <= 0) return;

    const std::size_t n = static_cast<std::size_t>(W) * H;
    auto& temp    = world.temperatureMap;
    auto& weather = world.weatherMap;
    temp.assign(n, 0.5f);
    weather.assign(n, 0.0f);

    const float sea = world.seaLevel;

    // Temperature: drops with altitude, warms near sea.
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float h = world.get(world.heightMap, x, z);
            float t = 1.0f - h;         // cooler up high
            if (h < sea + 0.05f) t += 0.1f; // mild sea warmth
            temp[static_cast<std::size_t>(z) * W + x] = std::clamp(t, 0.0f, 1.0f);
        }
    }

    // Classification.
    //   clear (0): humidity < 0.3
    //   snow  (2): humid AND cold
    //   rain  (1): humid AND warm
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            std::size_t i = static_cast<std::size_t>(z) * W + x;
            float hum = world.humidityMap[i];
            float T   = temp[i];
            if (hum < 0.3f) {
                weather[i] = 0.0f; // clear
            } else if (T < 0.3f) {
                weather[i] = 2.0f; // snow
            } else {
                weather[i] = 1.0f; // rain
            }
        }
    }
}

} // namespace WeatherMap
