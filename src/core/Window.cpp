#include <GLFW/glfw3.h>
#include "Window.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>

struct Window::Impl {
    GLFWwindow* window = nullptr;
    GLuint vao = 0, vbo = 0, shaderProgram = 0, texture = 0;

    Impl() {}
    ~Impl() {
        if (window) { glfwDestroyWindow(window); }
        glfwTerminate();
    }

    void init(int w, int h) {
        if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        window = glfwCreateWindow(w, h, "Terra Weather", nullptr, nullptr);
        if (!window) throw std::runtime_error("Failed to create window");
        
        glfwMakeContextCurrent(window);
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        // Fullscreen quad
        float vertices
bool Window::isKeyPressed(int key) const { return glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS; }
void Window::display(const float* fb, int w, int h) {
    if(fb) { glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, fb); }
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glfwSwapBuffers(glfwGetCurrentContext());
}
void Window::init(int w, int h) {}
