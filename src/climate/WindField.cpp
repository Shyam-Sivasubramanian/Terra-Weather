#include "Climate.h"
#include "WorldData.h"

#include <algorithm>
#include <vector>
#include <cmath>

namespace WindField {

void build(WorldData& world) {
    const int W = world.width;
    const int H = world.height;
    if (W <= 0 || H <= 0) return;

    std::vector<float>& U = world.windU;
    std::vector<float>& V = world.windV;
    const std::size_t n = static_cast<std::size_t>(W) * H;
    U.assign(n, 1.0f);
    V.assign(n, 0.0f);

    // Base prevailing wind: west-to-east.
    const float baseU = 1.0f;
    const float baseV = 0.0f;

    // Deflect wind around terrain using central differences in heightMap.
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float hL = world.get(world.heightMap, x - 1, z);
            float hR = world.get(world.heightMap, x + 1, z);
            float hD = world.get(world.heightMap, x, z - 1);
            float hU = world.get(world.heightMap, x, z + 1);
            float dhdx = (hR - hL) * 0.5f;
            float dhdz = (hU - hD) * 0.5f;

            // Higher altitude -> stronger deflection along gradient.
            float hCenter = world.get(world.heightMap, x, z);
            float altScale = std::clamp(hCenter * 2.5f, 0.0f, 1.2f);

            float u = baseU - dhdx * 1.5f * altScale;
            float v = baseV - dhdz * 1.5f * altScale;

            // Normalize magnitude softly to <= 1.
            float m = std::sqrt(u * u + v * v);
            if (m > 1.0f) { u /= m; v /= m; }

            U[static_cast<std::size_t>(z) * W + x] = u;
            V[static_cast<std::size_t>(z) * W + x] = v;
        }
    }

    // One box smoothing pass (3x3 avg).
    auto smooth = [&](std::vector<float>& M) {
        std::vector<float> out(M.size(), 0.0f);
        for (int z = 0; z < H; ++z) {
            for (int x = 0; x < W; ++x) {
                float s = 0.0f;
                for (int dz = -1; dz <= 1; ++dz)
                    for (int dx = -1; dx <= 1; ++dx)
                        s += world.get(M, x + dx, z + dz);
                out[static_cast<std::size_t>(z) * W + x] = s / 9.0f;
            }
        }
        M.swap(out);
    };
    smooth(U);
    smooth(V);
}

} // namespace WindField
