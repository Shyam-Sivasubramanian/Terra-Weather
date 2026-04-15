#include "Camera.h"
#include <cmath>

Camera::Camera()
    : position(0.0f, 0.0f, -1.0f)
    , lookAt(0.0f, 0.0f, 0.0f)
    , up(0.0f, 1.0f, 0.0f)
    , fov(60.0f)
    , aspectRatio(16.0f / 9.0f)
    , aperture(0.0f)
    , focusDistance(1.0f) {
    updateBasis();
}

void Camera::updateBasis() {
    // Calculate camera basis vectors
    w = glm::normalize(position - lookAt);  // Backward
    u = glm::normalize(glm::cross(up, w));  // Right
    v = glm::cross(w, u);                    // Up

    // Calculate image plane dimensions
    float theta = glm::radians(fov);
    halfHeight = tan(theta * 0.5f);
    halfWidth = aspectRatio * halfHeight;
}

Ray Camera::getRay(float s, float t) const {
    // s, t are normalized coordinates [0, 1]

    // Point on image plane
    glm::vec3 imagePoint = position - w * focusDistance
                          + (2.0f * s - 1.0f) * halfWidth * u
                          + (1.0f - 2.0f * t) * halfHeight * v;

    // Direction from camera to image point
    glm::vec3 direction = glm::normalize(imagePoint - position);

    return Ray(position, direction);
}

Ray Camera::getRayDOF(float s, float t, float uJitter, float vJitter) const {
    // Lens aperture sampling for depth of field
    glm::vec3 rd = aperture * randomInUnitDisk() * uJitter;
    glm::vec3 offset = u * rd.x + v * rd.y;

    // Point on image plane
    glm::vec3 imagePoint = position - w * focusDistance
                          + (2.0f * s - 1.0f) * halfWidth * u
                          + (1.0f - 2.0f * t) * halfHeight * v;

    // Direction from aperture to image point
    glm::vec3 direction = glm::normalize(imagePoint - position - offset);

    return Ray(position + offset, direction);
}

glm::vec3 Camera::randomInUnitDisk() const {
    // Simple random point in unit disk
    float r = sqrt(uJitter);  // Would be actual random in real implementation
    float theta = 6.2831853f * vJitter;
    return glm::vec3(r * cos(theta), r * sin(theta), 0.0f);
}
