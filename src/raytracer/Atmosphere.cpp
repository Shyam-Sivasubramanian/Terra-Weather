#include "Atmosphere.h"
#include "WorldData.h"
#include <cmath>
#include <algorithm>

/**
 * @brief Atmospheric scattering constants
 */
namespace AtmosphereConstants {
    // Rayleigh scattering coefficients (wavelength-dependent)
    const glm::vec3 rayleighCoeff = glm::vec3(5.8e-6f, 13.5e-6f, 33.1e-6f);

    // Mie scattering coefficient
    const float mieCoeff = 21e-6f;

    // Sun intensity
    const float sunIntensity = 22.0f;

    // Scale height for atmosphere
    const float rayleighHeight = 8000.0f;  // meters
    const float mieHeight = 1200.0f;        // meters

    // Planet radius and atmosphere thickness
    const float planetRadius = 6371000.0f;  // Earth radius in meters
    const float atmosphereRadius = 6471000.0f;  // Atmosphere outer radius
}

/**
 * @brief Atmospheric scattering implementation
 */
class Atmosphere::Impl {
public:
    const WorldData& worldData;
    glm::vec3 sunDirection;
    float turbidity;  // 1.0 = clear, 2.0 = hazy

    Impl(const WorldData& world) : worldData(world), sunDirection(0.0f, 1.0f, 0.0f), turbidity(1.0f) {}

    /**
     * @brief Calculate Rayleigh phase function
     */
    float rayleighPhase(float cosTheta) {
        return 3.0f / (16.0f * 3.14159265f) * (1.0f + cosTheta * cosTheta);
    }

    /**
     * @brief Calculate Mie phase function (Henyey-Greenstein)
     */
    float miePhase(float cosTheta, float g = 0.76f) {
        float g2 = g * g;
        return 3.0f / (8.0f * 3.14159265f) *
               ((1.0f - g2) * (1.0f + cosTheta * cosTheta)) /
               ((2.0f + g2) * pow(1.0f + g2 - 2.0f * g * cosTheta, 1.5f));
    }

    /**
     * @brief Calculate sky color using single-scattering approximation
     */
    glm::vec3 skyColor(const glm::vec3& rayDir, float time = 0.0f) {
        // Update sun direction based on time
        updateSunDirection(time);

        float cosTheta = glm::dot(rayDir, sunDirection);

        // Atmosphere color
        glm::vec3 atmosphereColor = glm::vec3(0.0f);

        // Rayleigh scattering (atmospheric particles)
        float rayleighTerm = rayleighPhase(cosTheta);
        atmosphereColor += rayleighTerm * AtmosphereConstants::rayleighCoeff;

        // Mie scattering (aerosols/water droplets)
        float mieTerm = miePhase(cosTheta);
        atmosphereColor += mieTerm * glm::vec3(AtmosphereConstants::mieCoeff * turbidity);

        // Sun intensity
        float sunFactor = glm::max(cosTheta, 0.0f);

        // Horizon gradient (blue to orange/red)
        float horizonFactor = 1.0f - rayDir.y;
        horizonFactor = pow(horizonFactor, 3.0f);

        glm::vec3 zenithColor(0.1f, 0.2f, 0.5f);
        glm::vec3 horizonColor(0.8f, 0.6f, 0.4f);

        glm::vec3 gradientColor = glm::mix(zenithColor, horizonColor, horizonFactor);

        // Combine scattering and gradient
        atmosphereColor = gradientColor * sunFactor;

        // Sun disk
        if (cosTheta > 0.9999f) {
            atmosphereColor += glm::vec3(10.0f, 8.0f, 6.0f) * (float)std::pow((cosTheta - 0.9999f) * 10000.0f, 2.0f);
        }

        return atmosphereColor;
    }

    /**
     * @brief Update sun direction based on time of day
     */
    void updateSunDirection(float time) {
        // Simple sun movement: time = 0..1 represents one day
        float angle = time * 6.2831853f - 1.5708f;  // Start at noon
        float sunHeight = sin(angle);

        if (sunHeight > 0.0f) {
            sunDirection = glm::normalize(glm::vec3(0.5f, sunHeight, 0.3f));
        } else {
            // Below horizon
            sunDirection = glm::normalize(glm::vec3(0.5f, sunHeight, 0.3f));
        }
    }

    /**
     * @brief Calculate atmospheric extinction along a ray
     */
    glm::vec3 atmosphericExtinction(const glm::vec3& start, const glm::vec3& end) {
        float distance = glm::length(end - start);

        // Beer-Lambert law
        float rayleighOpticalDepth = distance / AtmosphereConstants::rayleighHeight;
        float mieOpticalDepth = distance / AtmosphereConstants::mieHeight;

        glm::vec3 rayleighExtinction = exp(-rayleighOpticalDepth * AtmosphereConstants::rayleighCoeff);
        glm::vec3 mieExtinction = glm::exp(-mieOpticalDepth * glm::vec3(AtmosphereConstants::mieCoeff * turbidity));

        return rayleighExtinction * mieExtinction;
    }

    /**
     * @brief Calculate sky color with full atmospheric scattering
     */
    glm::vec3 skyColorFull(const glm::vec3& rayDir, const glm::vec3& sunDir, float time = 0.0f) {
        sunDirection = glm::normalize(sunDir);

        const int numSamples = 16;
        const int numLightSamples = 8;

        glm::vec3 rayOrigin(0.0f, 0.0f, 0.0f);
        float scaleHeight[2] = { AtmosphereConstants::rayleighHeight, AtmosphereConstants::mieHeight };
        glm::vec3 coefficients[2] = { AtmosphereConstants::rayleighCoeff, glm::vec3(AtmosphereConstants::mieCoeff * turbidity) };

        float mu = glm::dot(rayDir, sunDirection);
        float phaseR = rayleighPhase(mu);
        float phaseM = miePhase(mu);

        // Ray march through atmosphere
        float segmentLength = 100000.0f / numSamples;
        glm::vec3 opticalDepthR(0.0f), opticalDepthM(0.0f);
        glm::vec3 sumR(0.0f), sumM(0.0f);

        for (int i = 0; i < numSamples; i++) {
            glm::vec3 samplePos = rayOrigin + rayDir * (i + 0.5f) * segmentLength;
            float height = glm::length(samplePos) - AtmosphereConstants::planetRadius;

            // Optical depth for this sample
            float hr = exp(-height / scaleHeight[0]) * segmentLength;
            float hm = exp(-height / scaleHeight[1]) * segmentLength;
            opticalDepthR += hr;
            opticalDepthM += hm;

            // Light sampling (sun rays)
            float segmentLengthL = 100000.0f / numLightSamples;
            float opticalDepthLightR = 0.0f, opticalDepthLightM = 0.0f;

            for (int j = 0; j < numLightSamples; j++) {
                glm::vec3 samplePosL = samplePos + sunDir * (j + 0.5f) * segmentLengthL;
                float heightL = glm::length(samplePosL) - AtmosphereConstants::planetRadius;

                if (heightL < 0.0f) break;

                opticalDepthLightR += exp(-heightL / scaleHeight[0]) * segmentLengthL;
                opticalDepthLightM += exp(-heightL / scaleHeight[1]) * segmentLengthL;
            }

            // Accumulate scattering
            glm::vec3 tau = coefficients[0] * (opticalDepthR + opticalDepthLightR) +
                           coefficients[1] * 1.1f * (opticalDepthM + opticalDepthLightM);
            glm::vec3 attenuation = exp(-tau);

            sumR += attenuation * hr;
            sumM += attenuation * hm;
        }

        glm::vec3 color = sumR * phaseR * coefficients[0] + sumM * phaseM * coefficients[1];
        color *= AtmosphereConstants::sunIntensity;

        return color;
    }
};

/**
 * @brief Atmosphere Implementation
 */
Atmosphere::Atmosphere(const WorldData& world) : impl(std::make_unique<Impl>(world)) {}

Atmosphere::~Atmosphere() = default;

glm::vec3 Atmosphere::skyColor(const glm::vec3& rayDir, float time) {
    return impl->skyColor(rayDir, time);
}

void Atmosphere::setSunDirection(const glm::vec3& dir) {
    impl->sunDirection = glm::normalize(dir);
}

glm::vec3 Atmosphere::getSunDirection() const {
    return impl->sunDirection;
}

void Atmosphere::setTurbidity(float t) {
    impl->turbidity = t;
}

float Atmosphere::getTurbidity() const {
    return impl->turbidity;
}

glm::vec3 Atmosphere::atmosphericExtinction(const glm::vec3& start, const glm::vec3& end) {
    return impl->atmosphericExtinction(start, end);
}
