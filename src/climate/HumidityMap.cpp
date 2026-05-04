#include "Climate.h"
#include "WorldData.h"

#include <algorithm>
#include <vector>
#include <cmath>

namespace HumidityMap {

void build(WorldData& world) {
    const int W = world.width;
    const int H = world.height;
    if (W <= 0 || H <= 0) return;

    std::vector<float>& hum = world.humidityMap;
    hum.assign(static_cast<std::size_t>(W) * H, 0.5f);

    const float sea = world.seaLevel;

    // Base humidity: drier at higher elevations.
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float h = world.get(world.heightMap, x, z);
            float base = 1.0f - h;
            if (h < sea + 0.05f) base = 0.95f; // ocean boost
            hum[static_cast<std::size_t>(z) * W + x] = std::clamp(base, 0.0f, 1.0f);
        }
    }

    // Rain shadow: if western neighbor is higher by threshold, reduce.
    // Prevailing wind is west-to-east, so leeward east side is drier.
    std::vector<float> tmp = hum;
    for (int z = 0; z < H; ++z) {
        for (int x = 1; x < W; ++x) {
            float hw = world.get(world.heightMap, x - 1, z);
            float hc = world.get(world.heightMap, x,     z);
            if (hw > hc + 0.05f) {
                tmp[static_cast<std::size_t>(z) * W + x] -= 0.3f * (hw - hc) * 3.0f;
            }
        }
    }

    // 3x3 Gaussian blur: kernel [1,2,1;2,4,2;1,2,1]/16.
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float s =
                  1.0f * world.get(tmp, x-1, z-1) + 2.0f * world.get(tmp, x, z-1) + 1.0f * world.get(tmp, x+1, z-1)
                + 2.0f * world.get(tmp, x-1, z  ) + 4.0f * world.get(tmp, x, z  ) + 2.0f * world.get(tmp, x+1, z  )
                + 1.0f * world.get(tmp, x-1, z+1) + 2.0f * world.get(tmp, x, z+1) + 1.0f * world.get(tmp, x+1, z+1);
            hum[static_cast<std::size_t>(z) * W + x] = std::clamp(s / 16.0f, 0.0f, 1.0f);
        }
    }
}

} // namespace HumidityMap
