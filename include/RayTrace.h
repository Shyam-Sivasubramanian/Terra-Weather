#pragma once

#include <glm/glm.hpp>
#include "Ray.h"

class Scene;
class Atmosphere;
class VolumetricCloud;
class WeatherVolume;
struct WorldData;

namespace RayTrace {

struct Context {
    const Scene*           scene       = nullptr;
    const Atmosphere*      atmosphere  = nullptr;
    const VolumetricCloud* clouds      = nullptr;
    const WeatherVolume*   weather     = nullptr;
    const WorldData*       world       = nullptr;
    int   maxDepth        = 6;
    int   rrStartDepth    = 3;   // start Russian-roulette after this many bounces
    bool  directSunLight  = true;
    glm::vec3 sunDirectBoost{6.0f}; // sun radiance for direct-lighting path
};

// Trace one ray and return its radiance.
glm::vec3 trace(const Ray& r, const Context& ctx);

} // namespace RayTrace
