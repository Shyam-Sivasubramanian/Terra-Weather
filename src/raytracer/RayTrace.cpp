#include "RayTrace.h"
#include "Scene.h"
#include "Atmosphere.h"
#include "VolumetricCloud.h"
#include "WeatherVolume.h"
#include <algorithm>
#include <random>

/**
 * @brief Ray tracer configuration
 */
struct RayTracerConfig {
    int maxBounces = 8;           // Maximum ray bounce depth
    float rouletteThreshold = 0.1f;  // Russian roulette survival probability
    int rouletteStartBounce = 3;  // Start Russian roulette after this many bounces
    bool useSoftShadows = true;   // Enable soft shadow sampling
    int shadowRays = 1;            // Number of shadow rays per light
    bool useAmbientOcclusion = false;  // Enable AO (slow)
    int aoSamples = 0;             // AO sample count
};

/**
 * @brief Per-thread random generator
 */
struct ThreadRandom {
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;

    ThreadRandom() {
        std::random_device rd;
        rng.seed(rd());
    }

    ThreadRandom(uint32_t seed) {
        rng.seed(seed);
    }

    float random() {
        return dist(rng);
    }

    glm::vec3 randomInUnitSphere() {
        while (true) {
            float x = random() * 2.0f - 1.0f;
            float y = random() * 2.0f - 1.0f;
            float z = random() * 2.0f - 1.0f;
            glm::vec3 p(x, y, z);
            if (glm::dot(p, p) < 1.0f) return p;
        }
    }

    glm::vec3 randomCosineDirection() {
        float r1 = random();
        float r2 = random();
        float z = sqrt(1.0f - r1);
        float phi = 2.0f * 3.14159265f * r2;
        float x = cos(phi) * sqrt(r1);
        float y = sin(phi) * sqrt(r1);
        return glm::vec3(x, y, z);
    }
};

class RayTracer::Impl {
public:
    Impl(Scene& scene) : scene(scene), config(RayTracerConfig()) {
        atmosphere = std::make_unique<Atmosphere>(scene.getWorldData());
        clouds = std::make_unique<VolumetricCloud>(scene.getWorldData());
        weather = std::make_unique<WeatherVolume>(scene.getWorldData());
    }

    /**
     * @brief Trace a single ray through the scene
     *
     * Core ray tracing loop with:
     * - Surface intersection
     * - Material scattering
     * - Russian roulette termination
     * - Volume integration
     */
    glm::vec3 trace(const Ray& ray, float time = 0.0f) {
        ThreadRandom rng(static_cast<unsigned>(ray.origin.x * 1000 +
                                               ray.origin.z * 100 +
                                               time * 1000));

        glm::vec3 color = glm::vec3(0.0f);
        glm::vec3 throughput = glm::vec3(1.0f);

        Ray currentRay = ray;
        bool inVolume = false;

        for (int depth = 0; depth < config.maxBounces; depth++) {
            HitRecord rec;

            // Test intersection with scene
            bool hitSurface = scene.hit(currentRay, 0.001f, 1000.0f, rec);

            if (hitSurface) {
                // Material interaction
                glm::vec3 attenuation;
                Ray scattered;

                if (rec.material && rec.material->scatter(currentRay, rec, attenuation, scattered)) {
                    // Accumulate color from this bounce
                    color += throughput * attenuation;

                    // Check for emissive material (light source)
                    if (rec.material->isEmissive()) {
                        color += throughput * rec.material->emitted(rec.uv.x, rec.uv.y, rec.point);
                        break;
                    }

                    // Russian roulette for path termination
                    if (depth >= config.rouletteStartBounce) {
                        float p = glm::max(attenuation.r,
                                          glm::max(attenuation.g, attenuation.b));
                        if (p < config.rouletteThreshold) {
                            float survivalProb = p / config.rouletteThreshold;
                            if (rng.random() > survivalProb) {
                                break;  // Terminate path
                            }
                            throughput /= survivalProb;
                        }
                    }

                    // Continue tracing
                    throughput *= attenuation;
                    currentRay = scattered;

                    // Check for volume at hit point
                    float volumeDensity = scene.getWorldData().getCloudDensity(
                        static_cast<int>(rec.point.x * scene.getWorldData().width),
                        static_cast<int>(rec.point.z * scene.getWorldData().height)
                    );

                    if (volumeDensity > 0.001f && rec.point.y > 0.3f) {
                        inVolume = true;
                    }
                } else {
                    // Absorption (shouldn't happen with proper materials)
                    break;
                }
            } else {
                // Sky/atmosphere hit
                glm::vec3 atmosphereColor = atmosphere->skyColor(currentRay.direction, time);
                color += throughput * atmosphereColor;
                break;
            }

            // Check for volume scattering along ray
            if (inVolume) {
                // Ray march through volume
                glm::vec3 volumeColor = marchVolume(currentRay, time, rng);
                color += throughput * volumeColor * 0.1f;  // Volume contribution
            }

            // Early exit if throughput is negligible
            float maxThroughput = glm::max(throughput.r,
                                          glm::max(throughput.g, throughput.b));
            if (maxThroughput < 0.001f) break;
        }

        return color;
    }

    /**
     * @brief Trace with explicit volume marching
     */
    glm::vec3 traceWithVolume(const Ray& ray, float time = 0.0f) {
        ThreadRandom rng(static_cast<unsigned>(ray.origin.x * 1000 +
                                               ray.origin.z * 100 +
                                               time * 1000));

        glm::vec3 color = glm::vec3(0.0f);
        glm::vec3 throughput = glm::vec3(1.0f);

        float t = 0.0f;
        const float tMax = 1000.0f;
        const int steps = 64;
        float dt = tMax / steps;

        for (int i = 0; i < steps; i++) {
            glm::vec3 pos = ray.at(t);

            // Get volume density at this point
            float density = getVolumeDensity(pos, time);

            if (density > 0.001f) {
                // Sample volume
                glm::vec3 volumeColor = sampleVolume(pos, ray.direction, time, rng);

                // Beer-Lambert absorption
                float transmittance = exp(-density * dt * 2.0f);

                // Accumulate in-scattered light
                color += throughput * volumeColor * (1.0f - transmittance);

                // Update throughput
                throughput *= transmittance;

                if (glm::max(throughput.r, glm::max(throughput.g, throughput.b)) < 0.01f) {
                    break;
                }
            }

            // Check for surface intersection
            HitRecord rec;
            if (scene.hit(ray, t, t + dt, rec)) {
                // Surface hit - continue with surface shading
                color += throughput * shadeSurface(rec, ray.direction, time, rng);
                break;
            }

            t += dt;
        }

        // If no volume or surface hit, return sky color
        if (t >= tMax) {
            color += throughput * atmosphere->skyColor(ray.direction, time);
        }

        return color;
    }

    /**
     * @brief Get volume density at world position
     */
    float getVolumeDensity(const glm::vec3& pos, float time) const {
        const auto& world = scene.getWorldData();

        if (pos.y < 0.2f || pos.y > 0.95f) return 0.0f;

        int x = static_cast<int>(pos.x * world.width);
        int z = static_cast<int>(pos.z * world.height);

        return world.get(world.cloudDensity, x, z);
    }

    /**
     * @brief Sample volume at position
     */
    glm::vec3 sampleVolume(const glm::vec3& pos, const glm::vec3& dir,
                          float time, ThreadRandom& rng) const {
        // Cloud color based on sun position
        glm::vec3 sunDir = scene.getSunDirection();
        float sunAngle = glm::dot(dir, sunDir);

        // Simple cloud lighting
        glm::vec3 cloudColor = glm::mix(
            glm::vec3(0.3f, 0.35f, 0.4f),  // Shadowed cloud
            glm::vec3(1.0f, 0.98f, 0.95f),  // Lit cloud
            pow(glm::max(sunAngle, 0.0f), 2.0f)
        );

        return cloudColor;
    }

    /**
     * @brief March through volume accumulating scattering
     */
    glm::vec3 marchVolume(const Ray& ray, float time, ThreadRandom& rng) const {
        glm::vec3 pos = ray.origin;
        glm::vec3 result = glm::vec3(0.0f);

        const int steps = 16;
        float stepSize = 0.05f;

        for (int i = 0; i < steps; i++) {
            float density = getVolumeDensity(pos, time);
            if (density > 0.01f) {
                glm::vec3 sample = sampleVolume(pos, ray.direction, time, rng);
                float transmittance = exp(-density * stepSize);
                result += sample * density * stepSize;
                result *= transmittance;
            }
            pos += ray.direction * stepSize;
        }

        return result;
    }

    /**
     * @brief Shade surface hit
     */
    glm::vec3 shadeSurface(const HitRecord& rec, const glm::vec3& rayDir,
                          float time, ThreadRandom& rng) {
        glm::vec3 color = glm::vec3(0.0f);

        // Direct lighting
        glm::vec3 lightDir = scene.getSunDirection();
        float NdotL = glm::dot(rec.normal, lightDir);

        if (NdotL > 0.0f) {
            // Shadow test
            Ray shadowRay(rec.point + rec.normal * 0.001f, lightDir);
            HitRecord shadowRec;
            bool inShadow = scene.hit(shadowRay, 0.001f, 1000.0f, shadowRec);

            if (!inShadow) {
                color += scene.getSunColor() * scene.getSunIntensity() * NdotL;
            }
        }

        // Ambient
        color += scene.getAmbientLight();

        return color;
    }

    Scene& scene;
    RayTracerConfig config;
    std::unique_ptr<Atmosphere> atmosphere;
    std::unique_ptr<VolumetricCloud> clouds;
    std::unique_ptr<WeatherVolume> weather;
};

/**
 * @brief RayTracer implementation
 */
RayTracer::RayTracer(Scene& scene) : impl(std::make_unique<Impl>(scene)) {}

RayTracer::~RayTracer() = default;

glm::vec3 RayTracer::trace(const Ray& ray, float time) {
    return impl->trace(ray, time);
}

glm::vec3 RayTracer::traceWithVolume(const Ray& ray, float time) {
    return impl->traceWithVolume(ray, time);
}

void RayTracer::setMaxBounces(int bounces) {
    impl->config.maxBounces = bounces;
}

void RayTracer::setShadowRays(int rays) {
    impl->config.shadowRays = rays;
    impl->config.useSoftShadows = rays > 1;
}

/**
 * @brief Progressive photon mapping (optional enhancement)
 */
class ProgressivePhotonMapper {
public:
    void tracePhoton(const Ray& ray, const glm::vec3& flux) {
        // Photon tracing for global illumination
    }
};
