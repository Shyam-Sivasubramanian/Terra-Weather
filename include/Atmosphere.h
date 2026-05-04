#pragma once

#include <glm/glm.hpp>
#include "Ray.h"

class Scene;

class Atmosphere {
public:
    Atmosphere();

    // Angular sun position. elevation in radians above horizon; azimuth around +Y.
    void setSun(float elevationRad, float azimuthRad);
    void setSunFromTimeOfDay(float hours01); // 0..1, 0.5 = noon

    // Sky color for a ray that missed all geometry. HDR radiance (linear).
    glm::vec3 sampleSky(const Ray& r) const;

    // Sun direction (points from surface toward the sun), unit.
    glm::vec3 getSunDirection() const { return sunDir_; }

    // Sun color in HDR units; drives direct-illumination term.
    glm::vec3 getSunColor() const { return sunColor_; }

    // Visibility test: true if the ray from `point` toward the sun hits nothing.
    bool isInShadow(const glm::vec3& point, const Scene& scene) const;

    // Ground fog: extra extinction applied to rays below fogAltitude.
    float getFogDensity() const { return fogDensity_; }
    float getFogAltitude() const { return fogAltitude_; }

private:
    glm::vec3 sunDir_{0.0f, 1.0f, 0.0f};
    glm::vec3 sunColor_{1.0f};
    float fogDensity_  = 0.015f;
    float fogAltitude_ = 2.0f; // world-space Y below this, fog applies
};
