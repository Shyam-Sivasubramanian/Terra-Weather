#include "RayTrace.h"
#include "Scene.h"
#include "Atmosphere.h"
#include "VolumetricCloud.h"
#include "WeatherVolume.h"
#include "WorldData.h"
#include "Material.h"

#include <algorithm>
#include <cmath>

namespace RayTrace {

// Iterative implementation of the path tracer. Iterative (vs recursive) avoids
// blowing the stack at maxDepth and makes Russian-roulette cleaner.
glm::vec3 trace(const Ray& rIn, const Context& ctx) {
    Ray r = rIn;
    glm::vec3 throughput(1.0f);
    glm::vec3 radiance(0.0f);

    for (int depth = 0; depth < ctx.maxDepth; ++depth) {
        HitRecord rec;
        const float tFar = 1e5f;
        bool hit = ctx.scene && ctx.scene->hit(r, 1e-3f, tFar, rec);

        // Distance to geometry (or far clip if miss) — used by volume marches.
        float tHit = hit ? rec.t : tFar;

        // --- Volumetric contributions along this segment (cloud, weather) ---
        CloudResult cloudR;
        if (ctx.clouds && ctx.world && ctx.atmosphere) {
            cloudR = ctx.clouds->march(r, tHit, *ctx.world, *ctx.atmosphere);
        }
        WeatherResult wxR;
        if (ctx.weather && ctx.world) {
            wxR = ctx.weather->march(r, tHit, *ctx.world);
        }
        // Volume transmittance applies to everything further along the segment.
        float segT = cloudR.transmittance * wxR.transmittance;
        // In-scatter accumulates into radiance weighted by throughput-before-segment.
        radiance += throughput * (cloudR.inscattered + wxR.inscattered);

        if (!hit) {
            // Missed scene: add sky along this segment, attenuated by volumes.
            glm::vec3 sky = ctx.atmosphere ? ctx.atmosphere->sampleSky(r)
                                           : glm::vec3(0.5f, 0.7f, 1.0f);
            radiance += throughput * segT * sky;
            break;
        }

        // Apply volume transmittance before hit shading.
        throughput *= segT;

        // Emissive term (none for terrain, but keeps the loop general).
        if (rec.mat) radiance += throughput * rec.mat->emitted(rec);

        // --- Direct sun sampling ---
        // Trace a shadow ray toward the sun and add direct illumination.
        if (ctx.directSunLight && ctx.atmosphere) {
            glm::vec3 sunDir = ctx.atmosphere->getSunDirection();
            float nDotL = glm::dot(rec.normal, sunDir);
            if (nDotL > 0.0f) {
                Ray shadow(rec.point + rec.normal * 1e-3f, sunDir, 1e-3f, 1e5f);
                HitRecord srec;
                bool blocked = ctx.scene && ctx.scene->hit(shadow, 1e-3f, 1e5f, srec);
                if (!blocked) {
                    // Also attenuate direct sun by cloud transmittance sampled
                    // at a single midpoint. Cheap but visually plausible.
                    float sunT = 1.0f;
                    if (ctx.clouds && ctx.world) {
                        CloudResult cs = ctx.clouds->march(shadow, 50.0f,
                                                           *ctx.world, *ctx.atmosphere);
                        sunT = cs.transmittance;
                    }

                    // Scatter into eye from diffuse BRDF approximation:
                    // albedo * nDotL / pi — we approximate the albedo by letting
                    // the material scatter a probe ray and taking its attenuation.
                    glm::vec3 probeAtt(1.0f);
                    Ray dummyScatter;
                    if (rec.mat) rec.mat->scatter(r, rec, probeAtt, dummyScatter);
                    glm::vec3 sunRad = ctx.atmosphere->getSunColor() * ctx.sunDirectBoost;
                    radiance += throughput * probeAtt * sunRad * nDotL * sunT
                              * (1.0f / 3.14159265f);
                }
            }
        }

        // --- Indirect path continuation ---
        if (!rec.mat) break;
        glm::vec3 attenuation(1.0f);
        Ray scattered;
        if (!rec.mat->scatter(r, rec, attenuation, scattered)) break;
        throughput *= attenuation;
        r = scattered;

        // --- Russian roulette after rrStartDepth ---
        if (depth >= ctx.rrStartDepth) {
            float p = std::max({throughput.r, throughput.g, throughput.b});
            p = std::clamp(p, 0.05f, 0.95f);
            if (randFloat() > p) break;
            throughput /= p;
        }

        // Numerical safety: bail on NaNs or huge values.
        if (!std::isfinite(throughput.r) || !std::isfinite(throughput.g) ||
            !std::isfinite(throughput.b)) {
            break;
        }
    }

    return radiance;
}

} // namespace RayTrace
