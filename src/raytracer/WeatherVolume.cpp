#include "WeatherVolume.h"
#include "WorldData.h"
#include <cmath>
#include <algorithm>
#include <random>

/**
 * @brief Weather volume implementation for rain/snow effects
 */
class WeatherVolume::Impl {
public:
    const WorldData& worldData;

    // Weather parameters
    float rainStartHeight = 0.8f;   // Where rain/snow falls from
    float rainEndHeight = 0.3f;     // Where rain/snow stops (ground)
    float rainDensity = 0.0f;       // Current rain intensity [0,1]
    float snowDensity = 0.0f;       // Current snow intensity [0,1]
    float windSpeed = 1.0f;         // Wind affects particle fall angle
    float fallSpeed = 10.0f;        // Particle fall speed

    Impl(const WorldData& world) : worldData(world) {
        // Sample weather from world data
        updateFromWorldData();
    }

    void updateFromWorldData() {
        // Sample weather map to get rain/snow coverage
        float sumRain = 0.0f, sumSnow = 0.0f;
        int samples = 0;

        for (int z = 0; z < worldData.height; z += worldData.height / 8) {
            for (int x = 0; x < worldData.width; x += worldData.width / 8) {
                float weather = worldData.get(worldData.weatherMap, x, z);
                if (weather > 1.5f) {
                    sumSnow += (weather - 1.5f) * 2.0f;
                } else if (weather > 0.5f) {
                    sumRain += (weather - 0.5f) * 2.0f;
                }
                samples++;
            }
        }

        rainDensity = sumRain / samples;
        snowDensity = sumSnow / samples;

        // Sample wind
        float windUSum = 0.0f, windVSum = 0.0f;
        for (int i = 0; i < worldData.windU.size(); i += worldData.windU.size() / 16) {
            windUSum += std::abs(worldData.windU[i]);
            windVSum += std::abs(worldData.windV[i]);
        }
        windSpeed = sqrt(windUSum * windUSum + windVSum * windVSum) * 0.01f;
    }

    /**
     * @brief Sample weather density at position
     */
    float sampleDensity(const glm::vec3& pos, float time) const {
        if (pos.y > rainStartHeight || pos.y < rainEndHeight) return 0.0f;

        // Calculate fall distance (0 at top, 1 at bottom)
        float fallDist = (rainStartHeight - pos.y) / (rainStartHeight - rainEndHeight);

        // Wind offset
        float windOffset = time * windSpeed * 0.1f;

        // Rain drops (falling straight down with wind)
        float rainVal = 0.0f;
        if (rainDensity > 0.01f) {
            // Hash for pseudo-random rain
            float dropHash = hash(pos.x * 100 + windOffset, pos.z * 100 + windOffset * 0.5f);
            if (dropHash < rainDensity) {
                // Position in drop streak (creates streaks, not dots)
                float streakPhase = fmod(pos.x * 50 + pos.z * 30 + fallDist * 10, 1.0f);
                if (streakPhase < 0.3f) {
                    rainVal = rainDensity * (1.0f - streakPhase / 0.3f);
                }
            }
        }

        // Snow flakes (gentle falling, affected by wind more)
        float snowVal = 0.0f;
        if (snowDensity > 0.01f) {
            float flakeHash = hash(pos.x * 80 + windOffset * 2.0f,
                                  pos.z * 80 + windOffset * 1.5f + fallDist * 5.0f);
            if (flakeHash < snowDensity) {
                snowVal = snowDensity * 0.5f;
            }
        }

        return rainVal + snowVal;
    }

    /**
     * @brief Simple hash function
     */
    float hash(float x, float y) const {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        float ix = floor(x);
        float iy = floor(y);

        std::mt19937 rng(static_cast<unsigned>(ix * 1000 + iy * 100));
        return dis(rng);
    }

    /**
     * @brief Ray march through weather volume
     */
    glm::vec3 rayMarch(const Ray& ray, float time) const {
        if (rainDensity < 0.01f && snowDensity < 0.01f) {
            return glm::vec3(0.0f);
        }

        glm::vec3 color = glm::vec3(0.0f);
        float transmittance = 1.0f;

        // Find intersection with weather volume
        float tMin = 0.0f, tMax = 1000.0f;

        if (ray.origin.y < rainEndHeight && ray.direction.y <= 0) return glm::vec3(0.0f);
        if (ray.origin.y > rainStartHeight && ray.direction.y >= 0) return glm::vec3(0.0f);

        if (ray.origin.y < rainEndHeight) {
            tMin = (rainEndHeight - ray.origin.y) / ray.direction.y;
        }
        if (ray.origin.y > rainStartHeight) {
            tMin = (rainStartHeight - ray.origin.y) / ray.direction.y;
        }
        tMax = (rainEndHeight - ray.origin.y) / ray.direction.y;

        if (tMin < 0 || tMin > tMax) return glm::vec3(0.0f);

        // March
        const int steps = 32;
        float stepSize = (tMax - tMin) / steps;
        glm::vec3 sunDir = glm::normalize(glm::vec3(0.5f, 0.8f, 0.3f));

        for (int i = 0; i < steps; i++) {
            float t = tMin + (i + 0.5f) * stepSize;
            glm::vec3 pos = ray.origin + t * ray.direction;

            float density = sampleDensity(pos, time);

            if (density > 0.001f) {
                // Rain is blue-gray, snow is white
                glm::vec3 weatherColor = glm::vec3(0.5f, 0.55f, 0.6f);

                if (snowDensity > rainDensity) {
                    weatherColor = glm::vec3(0.95f, 0.97f, 1.0f);
                }

                // Light from sun
                float lightAngle = glm::dot(glm::normalize(pos - ray.origin), sunDir);
                weatherColor *= 0.5f + 0.5f * glm::max(lightAngle, 0.0f);

                // Accumulate
                float localTransmittance = exp(-density * stepSize * 5.0f);
                color += weatherColor * density * transmittance * stepSize;
                transmittance *= localTransmittance;

                if (transmittance < 0.01f) break;
            }
        }

        return color;
    }
};

/**
 * @brief WeatherVolume Implementation
 */
WeatherVolume::WeatherVolume(const WorldData& world)
    : impl(std::make_unique<Impl>(world)) {}

WeatherVolume::~WeatherVolume() = default;

glm::vec3 WeatherVolume::render(const Ray& ray, float time) {
    return impl->rayMarch(ray, time);
}

float WeatherVolume::density(const glm::vec3& pos, float time) {
    return impl->sampleDensity(pos, time);
}

void WeatherVolume::setWeatherIntensity(float rain, float snow) {
    impl->rainDensity = rain;
    impl->snowDensity = snow;
}

void WeatherVolume::setWindSpeed(float speed) {
    impl->windSpeed = speed;
}

float WeatherVolume::getRainIntensity() const {
    return impl->rainDensity;
}

float WeatherVolume::getSnowIntensity() const {
    return impl->snowDensity;
}

void WeatherVolume::update(const WorldData& world) {
    impl->updateFromWorldData();
}
