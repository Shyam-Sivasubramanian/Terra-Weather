#pragma once

#include <string>
#include <memory>

/**
 * @brief GLFW-based window with OpenGL display
 */
class Window {
public:
    /**
     * @brief Create window
     */
    Window(int width, int height, const std::string& title);
    ~Window();

    /**
     * @brief Display framebuffer to window
     */
    void display(const float* data, int width, int height);

    /**
     * @brief Check if window should close
     */
    bool shouldClose() const;

    /**
     * @brief Check if key is pressed
     */
    bool isKeyPressed(int key) const;

    /**
     * @brief Poll for events
     */
    void pollEvents();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
