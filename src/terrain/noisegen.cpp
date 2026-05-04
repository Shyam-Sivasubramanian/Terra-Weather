#include "NoiseGen.h"

#include <cmath>
#include <random>
#include <array>
#include <mutex>
#include <unordered_map>

namespace NoiseGen {

// Build a shuffled permutation table seeded deterministically.
// Cached per seed so repeat calls at the same seed are cheap.
static const std::array<int, 512>& permTable(unsigned int seed) {
    static std::mutex mtx;
    static std::unordered_map<unsigned int, std::array<int, 512>> cache;
    std::lock_guard<std::mutex> lock(mtx);
    auto it = cache.find(seed);
    if (it != cache.end()) return it->second;

    std::array<int, 256> p{};
    for (int i = 0; i < 256; ++i) p[i] = i;
    std::mt19937 gen(seed);
    for (int i = 255; i > 0; --i) {
        std::uniform_int_distribution<int> d(0, i);
        int j = d(gen);
        std::swap(p[i], p[j]);
    }
    std::array<int, 512> full{};
    for (int i = 0; i < 512; ++i) full[i] = p[i & 255];
    auto [ins, _] = cache.emplace(seed, full);
    return ins->second;
}

static inline float fade(float t)          { return t * t * t * (t * (t * 6 - 15) + 10); }
static inline float lerpf(float a, float b, float t) { return a + t * (b - a); }

static inline float grad2(int hash, float x, float y) {
    // 8 directions, 2D gradients on the corners of a square.
    switch (hash & 7) {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
        case 4: return  x;
        case 5: return -x;
        case 6: return  y;
        case 7: return -y;
    }
    return 0.0f;
}

static inline float grad3(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float perlin2D(float x, float y, unsigned int seed) {
    const auto& perm = permTable(seed);
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    x -= std::floor(x);
    y -= std::floor(y);
    float u = fade(x), v = fade(y);
    int A  = perm[X] + Y;
    int AA = perm[A];
    int AB = perm[A + 1];
    int B  = perm[X + 1] + Y;
    int BA = perm[B];
    int BB = perm[B + 1];

    float res = lerpf(
        lerpf(grad2(perm[AA],     x,     y    ),
              grad2(perm[BA],     x - 1, y    ), u),
        lerpf(grad2(perm[AB],     x,     y - 1),
              grad2(perm[BB],     x - 1, y - 1), u),
        v);
    // Classic 2D Perlin is bounded by ~sqrt(2)/2; normalize.
    return res * 1.4142f;
}

float perlin3D(float x, float y, float z, unsigned int seed) {
    const auto& perm = permTable(seed);
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    x -= std::floor(x); y -= std::floor(y); z -= std::floor(z);
    float u = fade(x), v = fade(y), w = fade(z);
    int A  = perm[X] + Y,  AA = perm[A] + Z,  AB = perm[A + 1] + Z;
    int B  = perm[X+1] + Y, BA = perm[B] + Z, BB = perm[B + 1] + Z;

    float res = lerpf(
        lerpf(lerpf(grad3(perm[AA  ], x,     y,     z    ),
                    grad3(perm[BA  ], x - 1, y,     z    ), u),
              lerpf(grad3(perm[AB  ], x,     y - 1, z    ),
                    grad3(perm[BB  ], x - 1, y - 1, z    ), u), v),
        lerpf(lerpf(grad3(perm[AA+1], x,     y,     z - 1),
                    grad3(perm[BA+1], x - 1, y,     z - 1), u),
              lerpf(grad3(perm[AB+1], x,     y - 1, z - 1),
                    grad3(perm[BB+1], x - 1, y - 1, z - 1), u), v),
        w);
    return res;
}

float fractal2D(float x, float y, int octaves, float persistence,
                float lacunarity, unsigned int seed) {
    float sum = 0.0f;
    float amp = 1.0f;
    float freq = 1.0f;
    float norm = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        sum  += amp * perlin2D(x * freq, y * freq, seed + i * 131u);
        norm += amp;
        amp  *= persistence;
        freq *= lacunarity;
    }
    return (norm > 0.0f) ? sum / norm : 0.0f;
}

std::vector<float> generate(int width, int height, unsigned int seed,
                            int octaves, float scale, float persistence,
                            float lacunarity) {
    std::vector<float> out(static_cast<std::size_t>(width) * height, 0.0f);
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            float fx = x * scale;
            float fz = z * scale;
            out[static_cast<std::size_t>(z) * width + x] =
                fractal2D(fx, fz, octaves, persistence, lacunarity, seed);
        }
    }
    return out;
}

} // namespace NoiseGen
