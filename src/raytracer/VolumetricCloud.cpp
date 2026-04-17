#include "VolumetricCloud.h"
#include "WorldData.h"
#include <cmath>
#include <algorithm>
#include <random>

/**
 * @brief Volumetric cloud rendering implementation
 */
class VolumetricCloud::Impl {
public:
    const WorldData& worldData;

    // Cloud parameters
    float cloudBase = 0.3f;       // Cloud layer bottom
    float cloudTop = 0.7f;        // Cloud layer top
    float densityScale = 1.0f;    // Overall density multiplier
    float absorption = 0.05f;     // Light absorption coefficient
    int marchSteps = 64;          // Ray march steps
    int lightSteps = 8;           // Light march steps per sample

    Impl(const WorldData& world) : worldData(world) {}

    /**
     * @brief Sample cloud density at world position
     */
    float sampleDensity(const glm::vec3& pos) const {
        if (pos.y < cloudBase || pos.y > cloudTop) return 0.0f;

        // Get height factor (0 at base, 1 at top)
        float heightFactor = (pos.y - cloudBase) / (cloudTop - cloudBase);

        // Base density from world data
        int x = static_cast<int>(pos.x * worldData.width);
        int z = static_cast<int>(pos.z * worldData.height);
        float baseDensity = worldData.get(worldData.cloudDensity, x, z);

        // Height gradient (clouds thicker in middle)
        float heightGradient = sin(heightFactor * 3.14159265f);

        // Add noise variation
        float noise = fbmNoise(pos.x * 10.0f, pos.y * 10.0f, pos.z * 10.0f);

        return baseDensity * heightGradient * (0.5f + 0.5f * noise) * densityScale;
    }

    /**
     * @brief 3D noise for cloud density variation
     */
    float noise3D(float x, float y, float z) const {
        // Simple value noise
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        float ix = floor(x);
        float iy = floor(y);
        float iz = floor(z);

        float fx = x - ix;
        float fy = y - iy;
        float fz = z - iz;

        // Hash function
        auto hash = [&](float x, float y, float z) -> float {
            std::mt19937 rng(static_cast<unsigned>(x * 1000 + y * 100 + z * 10));
            return dis(rng);
        };

        // Trilinear interpolation
        float v000 = hash(ix, iy, iz);
        float v100 = hash(ix + 1, iy, iz);
        float v010 = hash(ix, iy + 1, iz);
        float v110 = hash(ix + 1, iy + 1, iz);
        float v001 = hash(ix, iy, iz + 1);
        float v101 = hash(ix + 1, iy, iz + 1);
        float v011 = hash(ix, iy + 1, iz + 1);
        float v111 = hash(ix + 1, iy + 1, iz + 1);

        float x1 = v000 * (1 - fx) + v100 * fx;
        float x2 = v010 * (1 - fx) + v110 * fx;
        float x3 = v001 * (1 - fx) + v101 * fx;
        float x4 = v011 * (1 - fx) + v111 * fx;

        float y1 = x1 * (1 - fy) + x2 * fy;
        float y2 = x3 * (1 - fy) + x4 * fy;

        return y1 * (1 - fz) + y2 * fz;
    }

    /**
     * @brief Fractional Brownian Motion noise
     */
    float fbmNoise(float x, float y, float z, int octaves = 4) const {
        float value = 0.0f;
        float amplitude = 0.5f;
        float frequency = 1.0f;

        for (int i = 0; i < octaves; i++) {
            value += amplitude * noise3D(x * frequency, y * frequency, z * frequency);
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }

        return value;
    }

    /**
     * @brief Light march to calculate shadow
     */
    float lightMarch(const glm::vec3& pos, const glm::vec3& lightDir) const {
        float density = 0.0f;
        float stepSize = (cloudTop - pos.y) / lightSteps;

        for (int i = 0; i < lightSteps; i++) {
            glm::vec3 samplePos = pos + lightDir * stepSize * (float)i;
            density += sampleDensity(samplePos) * stepSize;
        }

        return exp(-density * absorption);
    }

    /**
     * @brief Ray march through cloud volume
     */
    glm::vec3 rayMarch(const Ray& ray, float time) const {
        glm::vec3 color = glm::vec3(0.0f);
        float transmittance = 1.0f;

        // Find entry/exit points
        float tMin = 0.0f, tMax = 1000.0f;

        // Calculate intersection with cloud layer
        if (ray.origin.y < cloudBase && ray.direction.y <= 0) return glm::vec3(0.0f);
        if (ray.origin.y > cloudTop && ray.direction.y >= 0) return glm::vec3(0.0f);

        if (ray.origin.y < cloudBase) {
            tMin = (cloudBase - ray.origin.y) / ray.direction.y;
        }
        if (ray.origin.y > cloudTop) {
            tMax = (cloudTop - ray.origin.y) / ray.direction.y;
        }

        if (tMin > tMax || tMin < 0) return glm::vec3(0.0f);

        // March through cloud
        float stepSize = (tMax - tMin) / marchSteps;
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, 0.8f, 0.3f));

        for (int i = 0; i < marchSteps; i++) {
            float t = tMin + (i + 0.5f) * stepSize;
            glm::vec3 pos = ray.origin + t * ray.direction;

            float density = sampleDensity(pos);

            if (density > 0.001f) {
                // Beer-Lambert
                transmittance *= exp(-density * stepSize * absorption);

                // Light scattering
                float lightDensity = lightMarch(pos, sunDir);

                // Cloud color (silver lining effect)
                float sunFacing = glm::dot(glm::normalize(pos - ray.origin), sunDir);
                sunFacing = glm::max(sunFacing, 0.0f);

                glm::vec3 cloudColor = glm::mix(
                    glm::vec3(0.5f, 0.55f, 0.6f),   // Shadowed cloud
                    glm::vec3(1.0f, 0.98f, 0.95f),   // Lit cloud
                    lightDensity * sunFacing
                );

                // Accumulate
                color += cloudColor * density * transmittance * stepSize;

                if (transmittance < 0.01f) break;
            }
        }

        return color * absorption;
    }
};

/**
 * @brief VolumetricCloud Implementation
 */
VolumetricCloud::VolumetricCloud(const WorldData& world)
    : impl(std::make_unique<Impl>(world)) {}

VolumetricCloud::~VolumetricCloud() = default;

glm::vec3 VolumetricCloud::render(const Ray& ray, float time) {
    return impl->rayMarch(ray, time);
}

float VolumetricCloud::density(const glm::vec3& pos) const {
    return impl->sampleDensity(pos);
}

void VolumetricCloud::setCloudHeight(float base, float top) {
    impl->cloudBase = base;
    impl->cloudTop = top;
}

void VolumetricCloud::setDensityScale(float scale) {
    impl->densityScale = scale;
}

void VolumetricCloud::setAbsorption(float absorption) {
    impl->absorption = absorption;
}

void VolumetricCloud::setMarchSteps(int steps, int lightSteps) {
    impl->marchSteps = steps;
    impl->lightSteps = lightSteps;
}
