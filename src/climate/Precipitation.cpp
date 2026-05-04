#include "Climate.h"
#include "WorldData.h"

#include <algorithm>
#include <vector>
#include <cmath>

namespace Precipitation {

// Semi-Lagrangian advection of humidity along wind, then re-run classify.
// Intensity is derived from humidity + classification at query time.
void update(WorldData& world, float dt) {
    const int W = world.width;
    const int H = world.height;
    if (W <= 0 || H <= 0 || dt <= 0.0f) return;

    std::vector<float> prev = world.humidityMap;
    const float speedScale = 0.5f; // how aggressively humidity drifts per second

    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float u = world.get(world.windU, x, z);
            float v = world.get(world.windV, x, z);
            float sx = x - u * speedScale * dt;
            float sz = z - v * speedScale * dt;

            // Bilinear sample in prev.
            int x0 = static_cast<int>(std::floor(sx));
            int z0 = static_cast<int>(std::floor(sz));
            float fx = sx - x0;
            float fz = sz - z0;
            float h00 = world.get(prev, x0,     z0    );
            float h10 = world.get(prev, x0 + 1, z0    );
            float h01 = world.get(prev, x0,     z0 + 1);
            float h11 = world.get(prev, x0 + 1, z0 + 1);
            float a   = h00 * (1 - fx) + h10 * fx;
            float b   = h01 * (1 - fx) + h11 * fx;
            float val = a * (1 - fz) + b * fz;

            // Slowly regenerate ocean moisture so the field doesn't drift to zero.
            float regen = 0.0f;
            if (world.get(world.heightMap, x, z) < world.seaLevel + 0.05f) {
                regen = (0.95f - val) * 0.1f * dt;
            }
            world.humidityMap[static_cast<std::size_t>(z) * W + x] =
                std::clamp(val + regen, 0.0f, 1.0f);
        }
    }

    // Re-classify weather after advection.
    WeatherMap::build(world);
}

float getIntensity(const WorldData& world, int x, int z) {
    float w = world.get(world.weatherMap, x, z);
    if (w < 0.5f) return 0.0f; // clear
    float hum = world.get(world.humidityMap, x, z);
    // Intensity scales with how far above 0.3 the humidity is.
    return std::clamp((hum - 0.3f) / 0.7f, 0.0f, 1.0f);
}

} // namespace Precipitation
