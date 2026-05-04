#pragma once

#include <glm/glm.hpp>
#include "Ray.h"

struct WorldData;
class Atmosphere;

struct CloudResult {
    float     transmittance = 1.0f;   // fraction of background light surviving
    glm::vec3 inscattered{0.0f};       // light scattered toward camera
};

class VolumetricCloud {
public:
    VolumetricCloud() = default;

    // March through the cloud altitude slab intersected by the ray.
    // Returns transmittance=1, inscattered=0 if the ray misses the slab.
    CloudResult march(const Ray& r,
                      float tHitGeometry,    // distance to next geometry hit (or large value)
                      const WorldData& world,
                      const Atmosphere& atmosphere) const;

    // Tunables (exposed in case ImGui wants to expose them later).
    float extinctionCoeff = 0.05f;
    int   stepCount       = 32;
    float hgG             = 0.4f; // Henyey-Greenstein asymmetry parameter
};
