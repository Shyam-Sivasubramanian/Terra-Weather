#pragma once

#include <glm/glm.hpp>
#include "Ray.h"

struct GLFWwindow;

class Camera {
public:
    Camera() = default;

    void init(const glm::vec3& pos,
              const glm::vec3& lookAt,
              const glm::vec3& up,
              float vfovDeg,
              float aspect,
              float aperture = 0.0f,
              float focusDist = 10.0f);

    void setAspect(float aspect);

    // Generate a primary ray through normalized image-plane coordinates (u, v) in [0,1].
    // u = 0 is left, v = 0 is TOP of image (so our framebuffer row 0 = top matches).
    Ray getRay(float u, float v) const;

    // Input handling. Call once per frame.
    // Returns true if the camera moved (i.e., framebuffer should reset).
    bool update(GLFWwindow* win, float deltaTime);

    const glm::vec3& position() const { return pos_; }

private:
    glm::vec3 pos_{0, 0, 0};
    glm::vec3 up_{0, 1, 0};
    float vfov_   = 60.0f;
    float aspect_ = 16.0f / 9.0f;
    float aperture_ = 0.0f;
    float focusDist_ = 10.0f;

    // Yaw/pitch in radians (yaw around +Y, pitch around local X).
    float yaw_   = 0.0f;
    float pitch_ = 0.0f;

    // Mouse look state.
    bool  mouseInit_ = false;
    double lastMouseX_ = 0.0, lastMouseY_ = 0.0;
    bool  rmbHeld_ = false;

    // Precomputed viewport basis (recomputed in recomputeBasis).
    glm::vec3 forward_{0, 0, -1};
    glm::vec3 right_  {1, 0,  0};
    glm::vec3 vup_    {0, 1,  0};
    glm::vec3 lowerLeft_{0};
    glm::vec3 horizontal_{0};
    glm::vec3 vertical_{0};
    float lensRadius_ = 0.0f;

    void recomputeBasis();
    static glm::vec3 randomInUnitDisk();
};
