#include "Atmosphere.h"
#include "Scene.h"

#include <algorithm>
#include <cmath>

static constexpr float kPi = 3.14159265358979323846f;

Atmosphere::Atmosphere() {
    setSunFromTimeOfDay(0.35f); // morning default
}

void Atmosphere::setSun(float elevationRad, float azimuthRad) {
    float ce = std::cos(elevationRad);
    sunDir_ = glm::normalize(glm::vec3(
        ce * std::cos(azimuthRad),
        std::sin(elevationRad),
        ce * std::sin(azimuthRad)));

    // Warmer/dimmer near horizon, white-gold at zenith.
    float h = std::max(0.0f, sunDir_.y);
    glm::vec3 warm(1.0f, 0.55f, 0.25f);
    glm::vec3 hot (1.0f, 0.95f, 0.85f);
    sunColor_ = glm::mix(warm, hot, h);
    // Dim at or below horizon.
    sunColor_ *= std::max(0.02f, h);
}

void Atmosphere::setSunFromTimeOfDay(float hours01) {
    // 0.0 = midnight, 0.5 = noon, 0.25 = sunrise, 0.75 = sunset.
    float angle = (hours01 - 0.25f) * 2.0f * kPi; // sunrise at angle 0
    float elevation = std::sin(angle);           // -1 at midnight, +1 at noon
    float elevationRad = std::asin(std::clamp(elevation, -1.0f, 1.0f));
    float azimuth = kPi; // due east-to-west progression simplified
    setSun(elevationRad, azimuth);
}

// Henyey-Greenstein phase function.
static inline float hg(float cosTheta, float g) {
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * kPi * std::pow(std::max(denom, 1e-4f), 1.5f));
}

// Rayleigh phase.
static inline float rayleighPhase(float cosTheta) {
    return (3.0f / (16.0f * kPi)) * (1.0f + cosTheta * cosTheta);
}

glm::vec3 Atmosphere::sampleSky(const Ray& r) const {
    glm::vec3 d = glm::normalize(r.direction);

    // Sun disk: a tight bright spot.
    float cosSun = glm::dot(d, sunDir_);
    // ~0.5 deg half-angle: cos(0.5 deg) ~= 0.99996; widen so it's visible.
    const float diskCos = 0.9985f;
    glm::vec3 sunDisk(0.0f);
    if (cosSun > diskCos && sunDir_.y > 0.0f) {
        float intensity = (cosSun - diskCos) / (1.0f - diskCos);
        sunDisk = sunColor_ * 30.0f * intensity;
    }

    // Sky color. Mix Rayleigh-ish blue with horizon warmth + Mie halo.
    glm::vec3 zenith(0.25f, 0.45f, 0.90f);
    glm::vec3 horizon(0.85f, 0.85f, 0.88f);
    glm::vec3 night (0.02f, 0.02f, 0.05f);

    // Blend day/night by sun height.
    float daylight = std::clamp(sunDir_.y * 2.0f + 0.2f, 0.0f, 1.0f);
    glm::vec3 skyDay = glm::mix(horizon, zenith, std::clamp(d.y, 0.0f, 1.0f));
    glm::vec3 sky    = glm::mix(night, skyDay, daylight);

    // Horizon darkening below the horizon line.
    if (d.y < 0.0f) {
        sky *= std::max(0.0f, 1.0f + d.y * 1.5f);
    }

    // Rayleigh + Mie scattering toward the sun.
    // Weighted sum using phase functions and simplistic in-scatter integrals.
    const glm::vec3 betaR(5.8e-6f, 13.5e-6f, 33.1e-6f); // per-meter
    const float    betaM = 3.996e-6f;
    float mu = cosSun;
    float scale = 3e5f; // empirical integration distance (meters)
    glm::vec3 rayleigh = betaR * rayleighPhase(mu) * scale * 20.0f;
    glm::vec3 mie      = glm::vec3(betaM) * hg(mu, 0.76f) * scale * 12.0f;
    glm::vec3 scattered = (rayleigh + mie) * sunColor_;

    // Attenuate scattered term above horizon; at night it should fade.
    scattered *= std::max(0.0f, sunDir_.y);

    return sky + scattered + sunDisk;
}

bool Atmosphere::isInShadow(const glm::vec3& point, const Scene& scene) const {
    if (sunDir_.y <= 0.0f) return true; // sun below horizon
    Ray shadow(point, sunDir_, 1e-3f, 1e5f);
    HitRecord rec;
    return scene.hit(shadow, 1e-3f, 1e5f, rec);
}
