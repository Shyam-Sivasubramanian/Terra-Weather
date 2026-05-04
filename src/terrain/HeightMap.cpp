#include "HeightMap.h"
#include "WorldData.h"
#include "NoiseGen.h"

#include <algorithm>
#include <cmath>

namespace HeightMap {

void build(WorldData& world) {
    if (world.width <= 0 || world.height <= 0) return;
    if (world.heightMap.size() != static_cast<std::size_t>(world.width) * world.height) {
        world.heightMap.assign(
            static_cast<std::size_t>(world.width) * world.height, 0.0f);
    }

    std::vector<float> raw = NoiseGen::generate(
        world.width, world.height, world.seed,
        /*octaves*/ 6, /*scale*/ 0.012f,
        /*persistence*/ 0.5f, /*lacunarity*/ 2.0f);

    // Find min/max for remap.
    float lo =  1e30f, hi = -1e30f;
    for (float v : raw) { lo = std::min(lo, v); hi = std::max(hi, v); }
    float range = std::max(hi - lo, 1e-6f);

    // Remap to [0,1], then apply a slight gamma to push mid values down
    // (more lowlands, fewer mountains — reads better visually).
    for (std::size_t i = 0; i < raw.size(); ++i) {
        float n = (raw[i] - lo) / range;
        n = std::pow(n, 1.25f);
        world.heightMap[i] = std::clamp(n, 0.0f, 1.0f);
    }

    world.seaLevel  = 0.35f;
    world.snowLevel = 0.75f;
}

void rebuild(WorldData& world, unsigned int seed) {
    world.seed = seed;
    build(world);
}

} // namespace HeightMap
