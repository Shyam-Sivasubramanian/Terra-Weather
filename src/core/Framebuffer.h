#pragma once

#include "glm/glm.hpp"
#include <vector>

/**
 * @brief CPU framebuffer for ray tracing output
 */
class Framebuffer {
public:
    Framebuffer(int width, int height);

    /**
     * @brief Set pixel color
     */
    void setPixel(int x, int y, const glm::vec3& color);

    /**
     * @brief Get pixel color
     */
    glm::vec3 getPixel(int x, int y) const;

    /**
     * @brief Clear framebuffer
     */
    void clear(const glm::vec3& color = glm::vec3(0.0f));

    /**
     * @brief Resize framebuffer
     */
    void resize(int width, int height);

    /**
     * @brief Get raw data pointer
     */
    const float* getData() const;

    /**
     * @brief Get dimensions
     */
    int getWidth() const;
    int getHeight() const;

private:
    int width, height;
    std::vector<float> data;
};
