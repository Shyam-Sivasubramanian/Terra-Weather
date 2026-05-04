#pragma once

#include <string>

struct GLFWwindow;
class Framebuffer;

class Window {
public:
    Window() = default;
    ~Window();

    bool init(int w, int h, const std::string& title = "ProceduralWorld");
    void shutdown();

    void pollEvents();
    bool shouldClose() const;

    // Upload CPU framebuffer and draw a fullscreen quad. Only OpenGL usage in project.
    void blit(Framebuffer& fb);
    void swap();

    int width()  const { return width_;  }
    int height() const { return height_; }

    GLFWwindow* handle() const { return window_; }

    // Reports if the window was resized since last call.
    bool consumeResized(int& w, int& h);

private:
    GLFWwindow* window_ = nullptr;
    int width_  = 0;
    int height_ = 0;

    unsigned int program_ = 0;
    unsigned int vao_     = 0;
    unsigned int vbo_     = 0;
    unsigned int texture_ = 0;
    int texW_ = 0, texH_ = 0;

    bool resized_ = false;

    bool compileBlitProgram();
    bool createQuad();
    void updateTexture(int w, int h, const float* pixels);

    static void framebufferSizeCallback(GLFWwindow* win, int w, int h);
};
