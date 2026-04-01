#pragma once

struct GLFWwindow;

class Window {
public:
    Window();
    ~Window();

    bool init(int width, int height, const char* title);
    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();

private:
    GLFWwindow* m_window;
};