#include <glad/glad.h>    // <--- FIX: GLAD MUST be included first!
#include <GLFW/glfw3.h>
#include "Window.h"
#include <iostream>

// (Include any other headers you had here)

struct Window::Impl {
    
    // ... (Keep any variables you had here, like your shaderProgram or VAO) ...

    void init(int w, int h) {
        // ---------------------------------------------------------
        //[YOUR EXISTING GLFW INIT & SHADER COMPILATION CODE GOES HERE]
        // (e.g., glfwInit, glfwCreateWindow, gladLoadGL, etc.)
        // ---------------------------------------------------------

    } // End of Window::Impl::init()

}; // <--- FIX: This closing bracket and semicolon were missing!

bool Window::isKeyPressed(int key) const { 
    return glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS; 
}

void Window::display(const float* fb, int w, int h) {
    // FIX: Only ONE copy of the display logic, safely inside the function
    if(fb) { 
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, fb); 
    }
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glfwSwapBuffers(glfwGetCurrentContext());
}

void Window::init(int w, int h) {
    // Delegate to the impl if you are using the Pimpl idiom
    // if(!impl) impl = new Impl();
    // impl->init(w, h);
}