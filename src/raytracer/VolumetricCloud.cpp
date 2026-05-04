#include "VolumetricCloud.h"
#include "WorldData.h"
#include "Atmosphere.h"
#include "CloudMap.h"

#include <algorithm>
#include <cmath>

static constexpr float kPi = 3.14159265358979323846f;

static inline float hg(float cosTheta, float g) {
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * kPi * std::pow(std::max(denom, 1e-4f), 1.5f));
}

// Intersect a ray with the horizontal slab y in [yMin, yMax].
// Returns false if the ray never enters the slab within [0, tMax].
static bool raySlab(const Ray& r, float yMin, float yMax,
                    float tMaxLimit, float& tEnter, float& tExit) {
    float oy = r.origin.y;
    float dy = r.direction.y;

    if (std::fabs(dy) < 1e-6f) {
        // Parallel to slab. Inside if origin within range.
        if (oy >= yMin && oy <= yMax) {
            tEnter = 0.0f;
            tExit  = tMaxLimit;
            return true;
        }
        return false;
    }
    float t0 = (yMin - oy) / dy;
    float t1 = (yMax - oy) / dy;
    if (t0 > t1) std::swap(t0, t1);
    tEnter = std::max(t0, 0.0f);
    tExit  = std::min(t1, tMaxLimit);
    return tExit > tEnter;
}

CloudResult VolumetricCloud::march(const Ray& r, float tHitGeometry,
                                   const WorldData& world,
                                   const Atmosphere& atmosphere) const {
    CloudResult out;
    if (world.cloudDensity.empty() || stepCount <= 0) return out;

    float yMin = world.cloudAltMin * world.heightScale;
    float yMax = world.cloudAltMax * world.heightScale;
    float tNear, tFar;
    if (!raySlab(r, yMin, yMax, tHitGeometry, tNear, tFar)) return out;
    if (tFar - tNear < 1e-3f) return out;

    float dt = (tFar - tNear) / stepCount;
    glm::vec3 sunDir = atmosphere.getSunDirection();
    glm::vec3 sunCol = atmosphere.getSunColor();
    float cosTheta = glm::dot(glm::normalize(r.direction), sunDir);
    float phase = hg(cosTheta, hgG);

    for (int i = 0; i < stepCount; ++i) {
        float t = tNear + (i + 0.5f) * dt;
        glm::vec3 p = r.at(t);
        float density = CloudMap::getCloudDensity(world, p.x, p.y, p.z);
        if (density <= 1e-4f) continue;

        float extinction = density * extinctionCoeff;
        float stepTrans  = std::exp(-extinction * dt);

        // Approximate sunlight arriving at p: attenuate by a short march along sunDir.
        // Cheap: single sample upward through the slab.
        float sunT = 1.0f;
        {
            Ray sr(p, sunDir, 1e-3f, 1e5f);
            float sNear, sFar;
            if (raySlab(sr, yMin, yMax, 200.0f, sNear, sFar)) {
                const int sSteps = 4;
                float sdt = (sFar - sNear) / sSteps;
                for (int j = 0; j < sSteps; ++j) {
                    float st = sNear + (j + 0.5f) * sdt;
                    glm::vec3 sp = sr.at(st);
                    float sd = CloudMap::getCloudDensity(world, sp.x, sp.y, sp.z);
                    sunT *= std::exp(-sd * extinctionCoeff * sdt);
                    if (sunT < 1e-3f) break;
                }
            }
        }

        // In-scatter from sun into the viewing ray.
        glm::vec3 inScat = sunCol * sunT * phase * extinction * dt;
        // Silver-lining boost when looking close to sun.
        if (cosTheta > 0.95f) inScat *= 1.0f + (cosTheta - 0.95f) * 10.0f;

        out.inscattered += out.transmittance * inScat;
        out.transmittance *= stepTrans;
        if (out.transmittance < 0.01f) { out.transmittance = 0.0f; break; }
    }

    return out;
}
