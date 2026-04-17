#pragma once

struct GLFWwindow;

class Window {
public:
    bool isKeyPressed(int key) const;
    void display(const float* fb, int w, int h);
    void init(int w, int h);
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