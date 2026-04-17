#pragma once
#include <memory>

class Window {
public:
    bool isKeyPressed(int key) const;
    void display(const float* fb, int w, int h);
    void init(int w, int h);
    bool shouldClose() const;
public:
    Window();
    ~Window();
    
    void init(int w, int h);
    void display(const float* fb, int w, int h);
    bool isKeyPressed(int key) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
