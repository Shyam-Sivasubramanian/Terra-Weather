#include "HeightMap.h"
<<<<<<< HEAD
#include "NoiseGen.h"

HeightMap::HeightMap(WorldData& world) : world(world) {}

void HeightMap::generate(const NoiseGen& noise) {
    int width = world.width;
    int height = world.height;

    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            // Normalized coordinates
            float nx = static_cast<float>(x) / width;
            float nz = static_cast<float>(z) / height;

            // Scale coordinates for noise
            float noiseX = nx * 8.0f;
            float noiseZ = nz * 8.0f;

            // Multi-octave terrain
            float terrain = noise.fbm(noiseX, noiseZ, 6, 2.0f, 0.5f);

            // Ridged mountains
            float mountains = noise.ridged(noiseX * 0.5f, noiseZ * 0.5f, 4, 0.5f);

            // Blend terrain and mountains
            float heightValue = terrain * 0.6f + mountains * 0.4f;

            // Islands: fade out at edges
            float edgeX = 1.0f - abs(2.0f * nx - 1.0f);
            float edgeZ = 1.0f - abs(2.0f * nz - 1.0f);
            float edgeFade = edgeX * edgeZ;
            edgeFade = pow(edgeFade, 0.5f);

            heightValue *= edgeFade;

            // Normalize to [0, 1]
            heightValue = (heightValue + 1.0f) * 0.5f;

            // Set sea level and snow level
            world.seaLevel = 0.35f;
            world.snowLevel = 0.85f;

            world.heightMap[z * width + x] = heightValue;
        }
    }

    std::cout << "Height map generated" << std::endl;
}

void HeightMap::applyErosion(int iterations, float erosionStrength) {
    // Simple thermal erosion
    for (int iter = 0; iter < iterations; iter++) {
        for (int z = 1; z < world.height - 1; z++) {
            for (int x = 1; x < world.width - 1; x++) {
                float center = world.heightMap[z * world.width + x];

                // Get neighbors
                float n = world.heightMap[(z - 1) * world.width + x];
                float s = world.heightMap[(z + 1) * world.width + x];
                float e = world.heightMap[z * world.width + x + 1];
                float w = world.heightMap[z * world.width + x - 1];

                // Calculate gradient
                float maxDiff = 0;
                float maxNeighbor = n;
                if (s - center > maxDiff) { maxDiff = s - center; maxNeighbor = s; }
                if (e - center > maxDiff) { maxDiff = e - center; maxNeighbor = e; }
                if (w - center > maxDiff) { maxDiff = w - center; maxNeighbor = w; }

                // Transfer material
                if (maxDiff > 0.01f) {
                    float transfer = maxDiff * erosionStrength * 0.25f;
                    world.heightMap[z * world.width + x] -= transfer;
                    world.heightMap[z * world.width + x] += transfer * 0.5f;
                }
            }
        }
    }
}

void HeightMap::normalize() {
    float minH = 1.0f, maxH = 0.0f;

    for (float h : world.heightMap) {
        minH = std::min(minH, h);
        maxH = std::max(maxH, h);
    }

    float range = maxH - minH;
    if (range > 0.001f) {
        for (float& h : world.heightMap) {
            h = (h - minH) / range;
        }
    }
}
=======
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
>>>>>>> fa2aca87a26a39699bf9f62c31cd09d43d385afd
