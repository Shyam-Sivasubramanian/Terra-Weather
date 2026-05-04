#pragma once

#include <glm/glm.hpp>
#include <random>
#include "Ray.h"
#include "HitRecord.h"

// ------------- Thread-local RNG helpers -------------
// Every ray-tracer thread needs its own RNG; std::rand is not thread-safe
// and mt19937 is cheap to keep per-thread.
inline std::mt19937& rngThread() {
    thread_local std::mt19937 gen{std::random_device{}()};
    return gen;
}

inline float randFloat() {
    thread_local std::uniform_real_distribution<float> d(0.0f, 1.0f);
    return d(rngThread());
}

inline float randFloat(float lo, float hi) {
    return lo + (hi - lo) * randFloat();
}

inline glm::vec3 randomInUnitSphere() {
    // Rejection sampling. Cheap and correct.
    while (true) {
        glm::vec3 p{randFloat(-1, 1), randFloat(-1, 1), randFloat(-1, 1)};
        if (glm::dot(p, p) < 1.0f) return p;
    }
}

inline glm::vec3 randomInUnitDisk() {
    while (true) {
        glm::vec3 p{randFloat(-1, 1), randFloat(-1, 1), 0.0f};
        if (glm::dot(p, p) < 1.0f) return p;
    }
}

inline glm::vec3 randomUnitVector() {
    return glm::normalize(randomInUnitSphere());
}

// True if vec is near zero in all dims (scatter direction safety check).
inline bool nearZero(const glm::vec3& v) {
    const float s = 1e-8f;
    return (std::fabs(v.x) < s) && (std::fabs(v.y) < s) && (std::fabs(v.z) < s);
}

// ------------- Material base -------------
class Material {
public:
    virtual ~Material() = default;

    // Returns true if the ray scatters; fills attenuation and the scattered ray.
    // Returns false for absorbed rays.
    virtual bool scatter(const Ray& in,
                         const HitRecord& rec,
                         glm::vec3& attenuation,
                         Ray& scattered) const = 0;

    // Emitted radiance at a hit point. Default: nothing (non-emissive).
    virtual glm::vec3 emitted(const HitRecord& /*rec*/) const {
        return glm::vec3(0.0f);
    }
};
