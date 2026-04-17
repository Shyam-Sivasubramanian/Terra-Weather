#pragma once
#include <memory>

class Window {
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
