#pragma once

#include <cmath>
#include <limits>
#include "glm/glm.hpp"

/**
 * @brief Ray structure for ray tracing
 *
 * Represents a ray with origin point and direction vector.
 * tmin and tmax define the valid interval along the ray for intersection tests.
 */
struct Ray {
    glm::vec3 origin;      // Ray origin point
    glm::vec3 direction;   // Ray direction (normalized)
    float tmin = 0.001f;   // Minimum valid t value (avoid self-intersection)
    float tmax = std::numeric_limits<float>::infinity();  // Maximum valid t value

    // Constructors
    Ray() : origin(0.0f), direction(0.0f, 0.0f, -1.0f) {}
    Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), direction(d) {}
    Ray(const glm::vec3& o, const glm::vec3& d, float tmin_, float tmax_)
        : origin(o), direction(d), tmin(tmin_), tmax(tmax_) {}

    /**
     * @brief Get point along ray at parameter t
     */
    glm::vec3 at(float t) const {
        return origin + t * direction;
    }

    /**
     * @brief Advance ray origin (for scattered rays)
     */
    void advance(float t) {
        origin = at(t);
    }
};

/**
 * @brief Inline helper functions for ray operations
 */
namespace RayUtils {
    /**
     * @brief Create a ray from camera parameters
     */
    inline Ray makeRay(const glm::vec3& origin, const glm::vec3& target,
                       const glm::vec3& up, float fov, float aspect,
                       float u, float v, float time = 0.0f) {
        // Camera basis vectors
        glm::vec3 w = glm::normalize(target - origin);
        glm::vec3 u_cam = glm::normalize(glm::cross(w, up));
        glm::vec3 v_cam = glm::cross(u_cam, w);

        // Ray direction using perspective projection
        float theta = glm::radians(fov);
        float halfHeight = tan(theta * 0.5f);
        float halfWidth = aspect * halfHeight;

        glm::vec3 lowerLeft = origin + w - halfWidth * u_cam - halfHeight * v_cam;
        glm::vec3 direction = glm::normalize(lowerLeft + 2.0f * halfWidth * u_cam * u + 2.0f * halfHeight * v_cam * v - origin);

        return Ray(origin, direction);
    }

    /**
     * @brief Create a ray with jittered origin for depth of field
     */
    inline Ray makeDOFRay(const glm::vec3& focusPoint, float focusDist,
                           const glm::vec3& apertureCenter, float apertureRadius,
                           const glm::vec3& u_cam, const glm::vec3& v_cam,
                           float u, float v, float time = 0.0f) {
        // Jitter aperture sample
        float r = sqrtf(u) * apertureRadius;
        float theta = v * 6.2831853f;
        glm::vec3 apertureOffset = r * cosf(theta) * u_cam + r * sinf(theta) * v_cam;

        glm::vec3 rayOrigin = apertureCenter + apertureOffset;
        glm::vec3 focusOffset = (focusDist / focusDist) * (focusPoint - apertureCenter);
        glm::vec3 focusTarget = apertureCenter + focusOffset;

        return Ray(rayOrigin, glm::normalize(focusTarget - rayOrigin));
    }

    /**
     * @brief Compute ray differential for texture LOD
     */
    inline void rayDifferential(Ray& ray, const glm::vec3& dOrigin, const glm::vec3& dDirection) {
        // For ray differentials, we need the change in origin and direction per pixel
        // This is used for texture filtering
    }
}  // namespace RayUtils
