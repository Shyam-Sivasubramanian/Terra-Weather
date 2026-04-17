#include <cmath>
#include <random>
#include <algorithm>
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>
#include "NoiseGen.h"
#include <cmath>
#include <algorithm>

/**
 * @brief Perlin noise generator
 */
class PerlinNoise {
public:
    PerlinNoise(unsigned int seed) {
        rng.seed(seed);

        // Initialize permutation table
        for (int i = 0; i < 256; i++) {
            p[i] = p[256 + i] = i;
        }

        // Shuffle
        for (int i = 255; i > 0; i--) {
            std::uniform_int_distribution<int> dist(0, i);
            int j = dist(rng);
            std::swap(p[i], p[j]);
        }
    }

    float noise(float x, float y) const {
        // Grid cell coordinates
        int xi = static_cast<int>(floor(x)) & 255;
        int yi = static_cast<int>(floor(y)) & 255;

        // Relative position within cell
        float xf = x - floor(x);
        float yf = y - floor(y);

        // Fade curves
        float u = fade(xf);
        float v = fade(yf);

        // Hash corners
        int aa = p[p[xi] + yi];
        int ab = p[p[xi] + yi + 1];
        int ba = p[p[xi + 1] + yi];
        int bb = p[p[xi + 1] + yi + 1];

        // Blend corners
        float x1 = lerp(grad(aa, xf, yf), grad(ba, xf - 1, yf), u);
        float x2 = lerp(grad(ab, xf, yf - 1), grad(bb, xf - 1, yf - 1), u);

        return lerp(x1, x2, v);
    }

    float octaveNoise(float x, float y, int octaves, float persistence) const {
        float total = 0.0f;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; i++) {
            total += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }

private:
    int p[512];
    std::mt19937 rng;

    static float fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    static float grad(int hash, float x, float y) {
        int h = hash & 3;
        float u = (h < 2) ? x : y;
        float v = (h < 2) ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }
};

/**
 * @brief NoiseGen Implementation
 */
NoiseGen::NoiseGen(unsigned int seed) : seed(seed) {}

float NoiseGen::perlin(float x, float y, int octaves, float persistence) const {
    PerlinNoise noise(seed);
    return noise.octaveNoise(x, y, octaves, persistence);
}

float NoiseGen::fbm(float x, float y, int octaves, float lacunarity, float persistence) const {
    PerlinNoise noise(seed);
    return noise.octaveNoise(x * lacunarity, y * lacunarity, octaves, persistence);
}

float NoiseGen::ridged(float x, float y, int octaves, float persistence, float ridgeOffset) const {
    PerlinNoise noise(seed);

    float sum = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;
    float prev = 1.0f;

    for (int i = 0; i < octaves; i++) {
        float n = 1.0f - abs(noise.octaveNoise(x * frequency, y * frequency, 1, 1.0f));
        n = n * n * (ridgeOffset + prev);
        sum += n * amplitude;
        maxValue += amplitude;
        frequency *= 2.0f;
        amplitude *= persistence;
        prev = n;
    }

    return sum / maxValue;
}

float NoiseGen::billow(float x, float y, int octaves, float persistence) const {
    return 2.0f * perlin(x, y, octaves, persistence) - 1.0f;
}

float NoiseGen::voronoi(float x, float y, float jitter) const {
    // Cell coordinates
    int xi = static_cast<int>(floor(x));
    int yi = static_cast<int>(floor(y));

    float minDist = 1.0f;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            std::mt19937 cellRng(seed + (xi + dx) * 1000 + (yi + dy) * 100);
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);

            float px = (xi + dx) + dist(cellRng) * jitter;
            float py = (yi + dy) + dist(cellRng) * jitter;

            float dist2 = (x - px) * (x - px) + (y - py) * (y - py);
            minDist = std::min(minDist, (float)std::sqrt(dist2));
        }
    }

    return minDist;
}
