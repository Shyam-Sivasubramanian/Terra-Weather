#pragma once

#include <glm/glm.hpp>
#include "Ray.h"

struct WorldData;

struct WeatherResult {
    float     transmittance = 1.0f;
    glm::vec3 inscattered{0.0f};
};

class WeatherVolume {
public:
    WeatherVolume() = default;

    // March from camera up to the next geometry hit (or far clip).
    WeatherResult march(const Ray& r,
                        float tHitGeometry,
                        const WorldData& world) const;

    int stepCount = 16;
};
