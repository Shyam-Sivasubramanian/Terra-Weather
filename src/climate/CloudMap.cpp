#include "CloudMap.h"
#include "WorldData.h"
#include "NoiseGen.h"

#include <algorithm>
#include <cmath>

namespace CloudMap {

// Vertical profile: bell curve peaking in the middle of the slab.
static inline float verticalProfile(int layer, int totalLayers) {
    if (totalLayers <= 1) return 1.0f;
    float t = static_cast<float>(layer) / (totalLayers - 1); // 0..1
    float x = t * 2.0f - 1.0f;                                // -1..1
    return std::max(0.0f, 1.0f - x * x);                      // bell curve
}

// smoothstep
static inline float ss(float a, float b, float v) {
    float t = std::clamp((v - a) / std::max(b - a, 1e-6f), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void build(WorldData& world) {
    const int W = world.width;
    const int H = world.height;
    const int L = world.cloudLayers;
    if (W <= 0 || H <= 0 || L <= 0) return;

    world.cloudDensity.assign(
        static_cast<std::size_t>(W) * H * L, 0.0f);

    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float hum = world.get(world.humidityMap, x, z);
            // Humidity threshold — very dry areas get no clouds.
            float hMix = ss(0.3f, 0.8f, hum);

            for (int l = 0; l < L; ++l) {
                float vp = verticalProfile(l, L);
                // Fractal noise perturbation for natural edges.
                float n = NoiseGen::perlin3D(
                    x * 0.05f,
                    l * 0.3f,
                    z * 0.05f,
                    world.seed + 7919u);
                n = 0.5f + 0.5f * n; // [0,1]
                float density = hMix * vp * n;
                density = std::clamp(density, 0.0f, 1.0f);
                world.cloudDensity[world.cloudIndex(x, l, z)] = density;
            }
        }
    }
}

// Map world-space Y to a floating-point layer index.
static inline float worldYToLayerF(const WorldData& w, float worldY) {
    float yMin = w.cloudAltMin * w.heightScale;
    float yMax = w.cloudAltMax * w.heightScale;
    if (yMax <= yMin) return -1.0f;
    float t = (worldY - yMin) / (yMax - yMin); // 0..1 inside slab
    return t * (w.cloudLayers - 1);
}

// Map world-space XZ to (gx, gz) float grid coords.
static inline void worldXZToGridF(const WorldData& w, float px, float pz,
                                  float& gx, float& gz) {
    gx = (px / w.worldScale + 0.5f) * (w.width  - 1);
    gz = (pz / w.worldScale + 0.5f) * (w.height - 1);
}

float getCloudDensity(const WorldData& world, float px, float py, float pz) {
    if (world.cloudDensity.empty()) return 0.0f;
    float layerF = worldYToLayerF(world, py);
    if (layerF < 0.0f || layerF > world.cloudLayers - 1) return 0.0f;

    float gx, gz;
    worldXZToGridF(world, px, pz, gx, gz);
    // Gracefully return 0 outside world footprint.
    if (gx < 0 || gz < 0 || gx > world.width - 1 || gz > world.height - 1) {
        return 0.0f;
    }

    int x0 = static_cast<int>(std::floor(gx));
    int z0 = static_cast<int>(std::floor(gz));
    int l0 = static_cast<int>(std::floor(layerF));
    float tx = gx - x0;
    float tz = gz - z0;
    float tl = layerF - l0;
    int x1 = std::min(x0 + 1, world.width  - 1);
    int z1 = std::min(z0 + 1, world.height - 1);
    int l1 = std::min(l0 + 1, world.cloudLayers - 1);
    x0 = std::clamp(x0, 0, world.width  - 1);
    z0 = std::clamp(z0, 0, world.height - 1);
    l0 = std::clamp(l0, 0, world.cloudLayers - 1);

    auto d = [&](int X, int L, int Z) {
        return world.cloudDensity[world.cloudIndex(X, L, Z)];
    };
    // Trilinear.
    float c000 = d(x0, l0, z0);
    float c100 = d(x1, l0, z0);
    float c010 = d(x0, l1, z0);
    float c110 = d(x1, l1, z0);
    float c001 = d(x0, l0, z1);
    float c101 = d(x1, l0, z1);
    float c011 = d(x0, l1, z1);
    float c111 = d(x1, l1, z1);
    float c00 = c000 * (1 - tx) + c100 * tx;
    float c10 = c010 * (1 - tx) + c110 * tx;
    float c01 = c001 * (1 - tx) + c101 * tx;
    float c11 = c011 * (1 - tx) + c111 * tx;
    float c0 = c00 * (1 - tl) + c10 * tl;
    float c1 = c01 * (1 - tl) + c11 * tl;
    return c0 * (1 - tz) + c1 * tz;
}

void advect(WorldData& world, float dt) {
    if (world.cloudDensity.empty() || dt <= 0.0f) return;
    const int W = world.width;
    const int H = world.height;
    const int L = world.cloudLayers;
    std::vector<float> prev = world.cloudDensity;

    const float speed = 0.8f;
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float u = world.get(world.windU, x, z);
            float v = world.get(world.windV, x, z);
            float sx = x - u * speed * dt;
            float sz = z - v * speed * dt;
            int x0 = static_cast<int>(std::floor(sx));
            int z0 = static_cast<int>(std::floor(sz));
            float fx = sx - x0;
            float fz = sz - z0;
            int x1 = x0 + 1, z1 = z0 + 1;
            auto clampX = [&](int xi) { return std::clamp(xi, 0, W - 1); };
            auto clampZ = [&](int zi) { return std::clamp(zi, 0, H - 1); };
            for (int l = 0; l < L; ++l) {
                auto sampleLayer = [&](int X, int Z) {
                    return prev[world.cloudIndex(clampX(X), l, clampZ(Z))];
                };
                float a = sampleLayer(x0, z0) * (1 - fx) + sampleLayer(x1, z0) * fx;
                float b = sampleLayer(x0, z1) * (1 - fx) + sampleLayer(x1, z1) * fx;
                float val = a * (1 - fz) + b * fz;
                world.cloudDensity[world.cloudIndex(x, l, z)] = val;
            }
        }
    }
}

} // namespace CloudMap
