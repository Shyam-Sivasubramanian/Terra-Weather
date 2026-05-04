#pragma once

#include <glm/glm.hpp>
#include "Ray.h"

class Material; // fwd

struct HitRecord {
    float t = 0.0f;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    bool frontFace = true;
    glm::vec2 uv{0.0f};
    const Material* mat = nullptr;

    // Ensure normal always points against the incoming ray.
    inline void setFaceNormal(const Ray& r, const glm::vec3& outwardNormal) {
        frontFace = glm::dot(r.direction, outwardNormal) < 0.0f;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};
