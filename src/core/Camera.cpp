#include "Camera.h"
#include "Material.h" // for randFloat helpers

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

static constexpr float kPi = 3.14159265358979323846f;

void Camera::init(const glm::vec3& pos,
                  const glm::vec3& lookAt,
                  const glm::vec3& up,
                  float vfovDeg,
                  float aspect,
                  float aperture,
                  float focusDist) {
    pos_       = pos;
    up_        = glm::normalize(up);
    vfov_      = vfovDeg;
    aspect_    = aspect;
    aperture_  = aperture;
    focusDist_ = focusDist;

    // Derive initial yaw/pitch from look direction.
    glm::vec3 dir = glm::normalize(lookAt - pos);
    pitch_ = std::asin(std::clamp(dir.y, -1.0f, 1.0f));
    yaw_   = std::atan2(dir.z, dir.x); // 0 yaw = +X direction

    recomputeBasis();
}

void Camera::setAspect(float aspect) {
    aspect_ = aspect;
    recomputeBasis();
}

void Camera::recomputeBasis() {
    // Clamp pitch to avoid gimbal flip at poles.
    const float limit = glm::radians(89.0f);
    pitch_ = std::clamp(pitch_, -limit, limit);

    forward_ = glm::normalize(glm::vec3(
        std::cos(pitch_) * std::cos(yaw_),
        std::sin(pitch_),
        std::cos(pitch_) * std::sin(yaw_)
    ));
    right_ = glm::normalize(glm::cross(forward_, up_));
    vup_   = glm::normalize(glm::cross(right_, forward_));

    float theta = glm::radians(vfov_);
    float h     = std::tan(theta * 0.5f);
    float viewportH = 2.0f * h;
    float viewportW = aspect_ * viewportH;

    horizontal_ = focusDist_ * viewportW * right_;
    vertical_   = focusDist_ * viewportH * vup_;
    // Lower-left corner of image plane when UV.y = 0 is TOP (so we negate vertical offset).
    // But getRay uses v in [0,1] top-to-bottom, so we actually compute corner at TOP-left
    // and subtract v*vertical. To keep the math symmetric we store the top-left corner here.
    // We'll treat `lowerLeft_` as top-left and v grows downward.
    lowerLeft_ = pos_
               + focusDist_ * forward_
               - 0.5f * horizontal_
               + 0.5f * vertical_;

    lensRadius_ = aperture_ * 0.5f;
}

glm::vec3 Camera::randomInUnitDisk() {
    // Use thread-safe RNG from Material.h.
    while (true) {
        glm::vec3 p(randFloat(-1.0f, 1.0f), randFloat(-1.0f, 1.0f), 0.0f);
        if (glm::dot(p, p) < 1.0f) return p;
    }
}

Ray Camera::getRay(float u, float v) const {
    glm::vec3 offset(0.0f);
    if (lensRadius_ > 0.0f) {
        glm::vec3 rd = lensRadius_ * randomInUnitDisk();
        offset = right_ * rd.x + vup_ * rd.y;
    }

    // top-left corner + u*right - v*down
    glm::vec3 target = lowerLeft_ + u * horizontal_ - v * vertical_;
    glm::vec3 dir = glm::normalize(target - pos_ - offset);
    return Ray(pos_ + offset, dir);
}

bool Camera::update(GLFWwindow* win, float deltaTime) {
    if (!win) return false;
    bool dirty = false;

    // Right-mouse look. Press & drag to look around.
    int rmb = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT);
    double mx, my; glfwGetCursorPos(win, &mx, &my);
    if (rmb == GLFW_PRESS) {
        if (!rmbHeld_) {
            rmbHeld_ = true;
            lastMouseX_ = mx; lastMouseY_ = my;
        } else {
            double dx = mx - lastMouseX_;
            double dy = my - lastMouseY_;
            lastMouseX_ = mx; lastMouseY_ = my;
            const float sens = 0.0025f;
            yaw_   += static_cast<float>(dx) * sens;
            pitch_ -= static_cast<float>(dy) * sens;
            if (dx != 0.0 || dy != 0.0) dirty = true;
        }
    } else {
        rmbHeld_ = false;
    }

    // WASD/QE movement in camera space.
    float speed = 15.0f;
    if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 3.0f;
    float dt = deltaTime;
    glm::vec3 move(0.0f);
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) move += forward_;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) move -= forward_;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) move += right_;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) move -= right_;
    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) move += up_;
    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) move -= up_;

    if (glm::dot(move, move) > 0.0f) {
        pos_ += glm::normalize(move) * speed * dt;
        dirty = true;
    }

    if (dirty) recomputeBasis();
    return dirty;
}
