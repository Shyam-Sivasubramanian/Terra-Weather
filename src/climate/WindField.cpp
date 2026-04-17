#include <cmath>
#include <cmath>
#include <iostream>
#include "WindField.h"

WindField::WindField(WorldData& world) : world(world) {}

void WindField::generate() {
    int width = world.width;
    int height = world.height;

    // Prevailing wind direction (west to east)
    float windAngle = 0.0f;  // Radians, 0 = from +X
    float windSpeed = 5.0f;

    for (int z = 0; z < height; z++) {
        for (int x = 0; x < width; x++) {
            float h = world.get(world.heightMap, x, z);

            // Base wind
            float u = cos(windAngle) * windSpeed;
            float v = sin(windAngle) * windSpeed;

            // Mountain deflection
            // Upwind slope increases wind
            // Downwind slope decreases wind (wind shadow)
            if (x > 0) {
                float hPrev = world.get(world.heightMap, x - 1, z);
                float slope = h - hPrev;

                // Deflect wind based on terrain
                if (slope > 0.05f) {
                    // Going up slope - slow down and deflect
                    u *= (1.0f - slope * 2.0f);
                    v += slope * 3.0f;
                } else if (slope < -0.05f) {
                    // Going down slope - speed up
                    u *= (1.0f - slope);
                }
            }

            // Turbulence near ground
            float turbulence = simpleNoise(x * 0.2f, z * 0.2f) * 0.5f;
            u += turbulence;
            v += turbulence * 0.7f;

            world.windU[z * width + x] = u;
            world.windV[z * width + x] = v;
        }
    }

    // Smooth wind field
    smoothWind();
}

void WindField::smoothWind() {
    int width = world.width;
    int height = world.height;
    int kernelSize = 2;

    std::vector<float> newU(world.windU.size());
    std::vector<float> newV(world.windV.size());

    for (int z = kernelSize; z < height - kernelSize; z++) {
        for (int x = kernelSize; x < width - kernelSize; x++) {
            float sumU = 0.0f, sumV = 0.0f;
            int count = 0;

            for (int dz = -kernelSize; dz <= kernelSize; dz++) {
                for (int dx = -kernelSize; dx <= kernelSize; dx++) {
                    sumU += world.windU[(z + dz) * width + (x + dx)];
                    sumV += world.windV[(z + dz) * width + (x + dx)];
                    count++;
                }
            }

            newU[z * width + x] = sumU / count;
            newV[z * width + x] = sumV / count;
        }
    }

    world.windU = newU;
    world.windV = newV;
}

float WindField::simpleNoise(float x, float y) const {
    return sin(x * 1.3f) * cos(y * 0.9f) * 0.5f;
}
