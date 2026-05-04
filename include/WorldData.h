#pragma once

#include <vector>
#include <algorithm>
#include <cstddef>

// Shared world state. Every subsystem reads from or writes to this struct.
// Shyam writes: heightMap, seaLevel, snowLevel.
// Janavi writes: humidityMap, temperatureMap, windU/V, weatherMap, cloudDensity.
// Renderer reads everything at ray-hit time.
struct WorldData {
    int width  = 0;
    int height = 0;
    unsigned int seed = 42;

    // --- Terrain (Shyam) ---
    std::vector<float> heightMap;          // [0,1] normalized elevation, size = width*height

    // --- Climate 2D fields (Janavi) ---
    std::vector<float> humidityMap;        // [0,1]
    std::vector<float> temperatureMap;     // [0,1]
    std::vector<float> windU;              // east-west wind, size = width*height
    std::vector<float> windV;              // north-south wind
    std::vector<float> weatherMap;         // 0=clear, 1=rain, 2=snow

    // --- Clouds: 3D density field flattened as [x + cloudLayers*(y_layer) + width*cloudLayers*z]
    // NOTE: indexing helper is cloudIndex() below.
    int cloudLayers = 16;
    float cloudAltMin = 0.6f;              // in normalized altitude
    float cloudAltMax = 0.9f;
    std::vector<float> cloudDensity;

    // --- Terrain parameters ---
    float seaLevel  = 0.35f;
    float snowLevel = 0.75f;

    // The world is rendered at this world-space extent; heightMap[i] is scaled by heightScale.
    float worldScale  = 100.0f;   // XZ extent in world units (terrain spans -worldScale/2..+worldScale/2)
    float heightScale = 20.0f;    // Y scale applied to heightMap values

    // --- Accessors ---
    // Safe 2D lookup with clamped boundary conditions.
    inline float get(const std::vector<float>& m, int x, int z) const {
        if (m.empty() || width <= 0 || height <= 0) return 0.0f;
        int cx = std::clamp(x, 0, width  - 1);
        int cz = std::clamp(z, 0, height - 1);
        return m[static_cast<std::size_t>(cz) * static_cast<std::size_t>(width) + cx];
    }

    // 3D cloud grid index: (x, layer, z) -> flat index.
    inline std::size_t cloudIndex(int x, int layer, int z) const {
        int cx = std::clamp(x,     0, width  - 1);
        int cl = std::clamp(layer, 0, cloudLayers - 1);
        int cz = std::clamp(z,     0, height - 1);
        return static_cast<std::size_t>(cz) * width * cloudLayers
             + static_cast<std::size_t>(cl) * width
             + static_cast<std::size_t>(cx);
    }

    // Resize all maps to width*height (cloudDensity to width*cloudLayers*height).
    void resizeAll(int w, int h) {
        width = w; height = h;
        const std::size_t n = static_cast<std::size_t>(w) * h;
        heightMap.assign(n, 0.0f);
        humidityMap.assign(n, 0.5f);
        temperatureMap.assign(n, 0.5f);
        windU.assign(n, 1.0f);
        windV.assign(n, 0.0f);
        weatherMap.assign(n, 0.0f);
        cloudDensity.assign(n * cloudLayers, 0.0f);
    }
};
