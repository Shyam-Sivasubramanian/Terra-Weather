#pragma once

#include "Ray.h"
#include "glm/glm.hpp"

/**
 * @brief Camera for ray generation
 */
class Camera {
public:
    glm::vec3 position;    // Camera position
    glm::vec3 lookAt;      // Look target
    glm::vec3 up;          // Up vector
    float fov;             // Field of view in degrees
    float aspectRatio;     // Width / Height
    float aperture;        // Aperture size for DOF
    float focusDistance;   // Focus distance

    Camera();

    /**
     * @brief Update camera basis vectors
     */
    void updateBasis();

    /**
     * @brief Generate ray for normalized coordinates
     */
    Ray getRay(float s, float t) const;

    /**
     * @brief Generate ray with depth of field
     */
    Ray getRayDOF(float s, float t, float uJitter, float vJitter) const;

    /**
     * @brief Set camera position
     */
    void setPosition(const glm::vec3& pos);

    /**
     * @brief Set look target
     */
    void setLookAt(const glm::vec3& target);

private:
    glm::vec3 u, v, w;  // Camera basis vectors
    float halfWidth, halfHeight;  // Image plane dimensions
    float uJitter, vJitter;  // For DOF

    glm::vec3 randomInUnitDisk() const;
};
