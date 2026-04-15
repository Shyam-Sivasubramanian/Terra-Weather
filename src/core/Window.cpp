#include "Window.h"
<<<<<<< HEAD
#include <glad/glad.h>
#include <GLFW/glfw3.h>

/**
 * @brief Window implementation using GLFW
 */
class Window::Impl {
public:
    GLFWwindow* window = nullptr;
    int width, height;
    std::string title;

    Impl(int w, int h, const std::string& t) : width(w), height(h), title(t) {
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Create window
        window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);

        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::runtime_error("Failed to initialize GLAD");
        }

        // Create fullscreen quad VAO
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        float vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Create texture
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Compile shaders
        compileShaders();
    }

    ~Impl() {
        if (window) glfwDestroyWindow(window);
    }

    void compileShaders() {
        // Vertex shader
        const char* vertSrc = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoord;
            void main() {
                gl_Position = vec4(aPos, 1.0);
                TexCoord = aTexCoord;
            }
        )";

        // Fragment shader (display texture)
        const char* fragSrc = R"(
            #version 330 core
            in vec2 TexCoord;
            out vec4 FragColor;
            uniform sampler2D tex;
            void main() {
                FragColor = texture(tex, TexCoord);
            }
        )";

        unsigned int vertShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertShader, 1, &vertSrc, nullptr);
        glCompileShader(vertShader);

        unsigned int fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragShader, 1, &fragSrc, nullptr);
        glCompileShader(fragShader);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertShader);
        glAttachShader(shaderProgram, fragShader);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        glUseProgram(shaderProgram);
        glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
    }

    void display(const float* data, int w, int h) {
        // Update texture
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, data);

        // Draw
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glfwSwapBuffers(window);
    }

    bool shouldClose() const {
        return window && glfwWindowShouldClose(window);
    }

    bool isKeyPressed(int key) const {
        return window && glfwGetKey(window, key) == GLFW_PRESS;
    }

    void pollEvents() {
        glfwPollEvents();
    }

private:
    unsigned int vao, vbo, texture, shaderProgram;
};

Window::Window(int w, int h, const std::string& title)
    : impl(std::make_unique<Impl>(w, h, title)) {}

Window::~Window() = default;

void Window::display(const float* data, int w, int h) {
    impl->display(data, w, h);
}

bool Window::shouldClose() const {
    return impl->shouldClose();
}

bool Window::isKeyPressed(int key) const {
    return impl->isKeyPressed(key);
}

void Window::pollEvents() {
    impl->pollEvents();
}
=======
#include <GLFW/glfw3.h>
#include <iostream>

Window::Window() : m_window(nullptr) {}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool Window::init(int width, int height, const char* title) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    return true;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}
>>>>>>> fa2aca87a26a39699bf9f62c31cd09d43d385afd
