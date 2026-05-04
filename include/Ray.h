#pragma once

#include <glm/glm.hpp>

struct Ray {
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};
    float tmin = 1e-3f;
    float tmax = 1e9f;

    Ray() = default;
    Ray(const glm::vec3& o, const glm::vec3& d, float tn = 1e-3f, float tf = 1e9f)
        : origin(o), direction(d), tmin(tn), tmax(tf) {}

    inline glm::vec3 at(float t) const { return origin + t * direction; }
};
