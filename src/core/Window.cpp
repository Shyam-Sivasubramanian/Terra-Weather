#include "Window.h"
#include "Framebuffer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>

// Vertex shader: emit fullscreen quad, pass UV.
static const char* kVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// Fragment shader: sample LDR texture and output. Tone-map & gamma happen CPU-side.
static const char* kFragSrc = R"(
#version 330 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
void main() {
    FragColor = vec4(texture(uTex, vUV).rgb, 1.0);
}
)";

Window::~Window() { shutdown(); }

void Window::framebufferSizeCallback(GLFWwindow* win, int w, int h) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
    if (!self) return;
    self->width_  = w;
    self->height_ = h;
    self->resized_ = true;
    glViewport(0, 0, w, h);
}

bool Window::init(int w, int h, const std::string& title) {
    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
    if (!window_) {
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "gladLoadGL failed\n");
        return false;
    }

    width_ = w; height_ = h;
    glViewport(0, 0, w, h);

    if (!compileBlitProgram()) return false;
    if (!createQuad()) return false;

    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return true;
}

bool Window::compileBlitProgram() {
    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            std::fprintf(stderr, "shader compile err: %s\n", log);
            glDeleteShader(s);
            return 0;
        }
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER,   kVertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFragSrc);
    if (!vs || !fs) return false;

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    GLint ok = 0; glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::fprintf(stderr, "program link err: %s\n", log);
        glDeleteShader(vs); glDeleteShader(fs);
        return false;
    }
    glDeleteShader(vs); glDeleteShader(fs);
    return true;
}

bool Window::createQuad() {
    // Two triangles covering NDC [-1,1]^2; UVs [0,1]^2.
    // Note on UV flip: we upload the framebuffer top-to-bottom same as pixel order,
    // so UV.y = 0 at top. Using row 0 = top matches.
    float verts[] = {
        // pos       // uv
        -1.f, -1.f,  0.f, 1.f,
         1.f, -1.f,  1.f, 1.f,
         1.f,  1.f,  1.f, 0.f,

        -1.f, -1.f,  0.f, 1.f,
         1.f,  1.f,  1.f, 0.f,
        -1.f,  1.f,  0.f, 0.f,
    };
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    return true;
}

void Window::updateTexture(int w, int h, const float* pixels) {
    glBindTexture(GL_TEXTURE_2D, texture_);
    if (w != texW_ || h != texH_) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, pixels);
        texW_ = w; texH_ = h;
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, pixels);
    }
}

void Window::blit(Framebuffer& fb) {
    const float* pixels = fb.uploadPixels();
    int fw = fb.width();
    int fh = fb.height();
    if (fw <= 0 || fh <= 0 || !pixels) return;

    updateTexture(fw, fh, pixels);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_);
    GLint loc = glGetUniformLocation(program_, "uTex");
    if (loc >= 0) glUniform1i(loc, 0);

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

void Window::swap() {
    if (window_) glfwSwapBuffers(window_);
}

void Window::pollEvents() { glfwPollEvents(); }

bool Window::shouldClose() const {
    return window_ ? glfwWindowShouldClose(window_) : true;
}

bool Window::consumeResized(int& w, int& h) {
    if (!resized_) return false;
    w = width_;
    h = height_;
    resized_ = false;
    return true;
}

void Window::shutdown() {
    if (texture_) { glDeleteTextures(1, &texture_); texture_ = 0; }
    if (vbo_)     { glDeleteBuffers(1, &vbo_);       vbo_ = 0; }
    if (vao_)     { glDeleteVertexArrays(1, &vao_);  vao_ = 0; }
    if (program_) { glDeleteProgram(program_);       program_ = 0; }
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        glfwTerminate();
    }
}
