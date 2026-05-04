#pragma once

#include <vector>
#include <cstdint>

namespace NoiseGen {

// Raw Perlin in [-1,1].
float perlin2D(float x, float y, unsigned int seed);

// Sum of octaves with per-octave amplitude / frequency.
float fractal2D(float x, float y, int octaves, float persistence,
                float lacunarity, unsigned int seed);

// Produce a width*height raw grid in [-1,1] (row-major, row 0 top).
// `scale` = base frequency for first octave (larger = more zoomed in).
std::vector<float> generate(int width, int height, unsigned int seed,
                            int octaves = 6, float scale = 0.012f,
                            float persistence = 0.5f, float lacunarity = 2.0f);

// 3D Perlin (used for cloud density perturbation). Returns [-1,1].
float perlin3D(float x, float y, float z, unsigned int seed);

} // namespace NoiseGen
