#include "WeatherVolume.h"
#include "WorldData.h"
#include "Climate.h"

#include <algorithm>
#include <cmath>

static inline void worldXZToGrid(const WorldData& w, float px, float pz,
                                 int& gx, int& gz) {
    float fx = (px / w.worldScale + 0.5f) * (w.width  - 1);
    float fz = (pz / w.worldScale + 0.5f) * (w.height - 1);
    gx = std::clamp(static_cast<int>(std::floor(fx + 0.5f)), 0, w.width  - 1);
    gz = std::clamp(static_cast<int>(std::floor(fz + 0.5f)), 0, w.height - 1);
}

WeatherResult WeatherVolume::march(const Ray& r,
                                   float tHitGeometry,
                                   const WorldData& world) const {
    WeatherResult out;
    if (world.weatherMap.empty() || stepCount <= 0) return out;

    // March from origin to tHitGeometry (but cap at a sensible ceiling for sky rays).
    float tMax = std::min(tHitGeometry, 200.0f);
    if (tMax <= 1e-3f) return out;

    float dt = tMax / stepCount;
    for (int i = 0; i < stepCount; ++i) {
        float t = (i + 0.5f) * dt;
        glm::vec3 p = r.at(t);

        // Rain/snow only below cloud base (no precip up in the stratosphere).
        float cloudFloor = world.cloudAltMin * world.heightScale;
        if (p.y > cloudFloor) continue;

        int gx, gz;
        worldXZToGrid(world, p.x, p.z, gx, gz);
        float w = world.get(world.weatherMap, gx, gz);
        if (w < 0.5f) continue; // clear

        float intensity = Precipitation::getIntensity(world, gx, gz);
        if (intensity <= 0.0f) continue;

        float density;
        glm::vec3 tint;
        if (w > 1.5f) {
            // Snow: denser, brighter, more scattering.
            density = intensity * 0.04f;
            tint    = glm::vec3(0.95f, 0.97f, 1.0f);
        } else {
            // Rain: thinner, slightly blue.
            density = intensity * 0.02f;
            tint    = glm::vec3(0.55f, 0.65f, 0.80f);
        }

        float stepTrans = std::exp(-density * dt);
        glm::vec3 inScat = tint * density * dt * 0.8f;

        out.inscattered += out.transmittance * inScat;
        out.transmittance *= stepTrans;
        if (out.transmittance < 0.02f) { out.transmittance = 0.0f; break; }
    }

    return out;
}
