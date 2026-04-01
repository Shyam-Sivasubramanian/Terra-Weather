#include "HeightMap.h"
#include <cmath>

namespace HeightMap {
    void build(WorldData& worldData) {
        worldData.heightMap.resize(worldData.width * worldData.height);

        for (int z = 0; z < worldData.height; ++z) {
            for (int x = 0; x < worldData.width; ++x) {
                float fx = static_cast<float>(x) / worldData.width;
                float fz = static_cast<float>(z) / worldData.height;

                float value = 0.5f + 0.25f * std::sin(fx * 10.0f) * std::cos(fz * 10.0f);
                worldData.heightMap[z * worldData.width + x] = value;
            }
        }

        worldData.seaLevel = 0.35f;
        worldData.snowLevel = 0.75f;
    }
}