#include <stdexcept>
#include <glad/glad.h>
#include "Window.h"
#define GLAD_GL_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <dlfcn.h>

// Simple OpenGL function loader
static void* opengl_handle = nullptr;

static void* getGLProcAddress(const char* name) {
    if (!opengl_handle) {
        opengl_handle = dlopen("libOpenGL.so", RTLD_NOW);
        if (!opengl_handle) {
            opengl_handle = dlopen("libGL.so.1", RTLD_NOW);
        }
        if (!opengl_handle) {
            opengl_handle = dlopen("libGL.so", RTLD_NOW);
        }
    }
    if (opengl_handle) {
        return dlsym(opengl_handle, name);
    }
    return nullptr;
}

#define GLAD_GL 1
#define GLAD_GL_IMPLEMENTATION

// Include minimal glad for GL 3.3
#define __gl_h_
#define __glext_h_

typedef int GLint;
typedef unsigned int GLuint;
// ;
typedef intptr_t GLintptr;
typedef unsigned int GLenum;
typedef float GLfloat;
typedef int GLsizei;
typedef void GLvoid;
typedef double GLdouble;
typedef unsigned int GLbitfield;
typedef unsigned char GLubyte;
typedef char GLchar;

#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03

// CLEARPROC)(GLbitfield);
// CLEARCOLORPROC)(GLfloat,GLfloat,GLfloat,GLfloat);
// ENABLEPROC)(GLenum);
// DISABLEPROC)(GLenum);
// DRAWARRAYSPROC)(GLenum,GLint,GLsizei);
// BINDTEXTUREPROC)(GLenum,GLuint);
// ACTIVETEXTUREPROC)(GLenum);
// BINDVERTEXARRAYPROC)(GLuint);
// BINDBUFFERPROC)(GLenum,GLuint);
// ,const void*,GLenum);
// DRAWRANGEELEMENTSPROC)(GLenum,GLuint,GLuint,GLsizei,GLenum,const void*);
// GETERRORPROC)(void);
// GETINTEGERVPROC)(GLenum,GLint*);
// GETSTRINGPROC)(GLenum);
// UNIFORM1IPROC)(GLint,GLint);
// UNIFORM1FPROC)(GLint,GLfloat);
// UNIFORM2FPROC)(GLint,GLfloat,GLfloat);
// UNIFORM3FPROC)(GLint,GLfloat,GLfloat,GLfloat);
// UNIFORM4FPROC)(GLint,GLfloat,GLfloat,GLfloat,GLfloat);
// GETUNIFORMLOCATIONPROC)(GLuint,const GLchar*);
// USEPROGRAMPROC)(GLuint);
// DELETEBUFFERSPROC)(GLsizei,const GLuint*);
// DELETETEXTURESPROC)(GLsizei,const GLuint*);
// DELETEVERTEXARRAYSPROC)(GLsizei,const GLuint*);
// DELETEPROGRAMPROC)(GLuint);
// DELETESHADERPROC)(GLuint);
// ATTACHSHADERPROC)(GLuint,GLuint);
// DETACHSHADERPROC)(GLuint,GLuint);
// LINKPROGRAMPROC)(GLuint);
// PIXELSTOREIPROC)(GLenum,GLint);
// TEXPARAMETERIPROC)(GLenum,GLenum,GLint);
// VERTEXATTRIBPOINTERPROC)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
// ENABLEVERTEXATTRIBARRAYPROC)(GLuint);
// DISABLEVERTEXATTRIBARRAYPROC)(GLuint);
// CREATESHADERPROC)(GLenum);
typedef void (APIENTRYP PFNGCSHADERSOURCEPROC)(GLuint,GLsizei,const GLchar*const*,const GLint*);
// COMPILESHADERPROC)(GLuint);
// GETSHADERIVPROC)(GLuint,GLenum,GLint*);
// GETSHADERINFOLOGPROC)(GLuint,GLsizei,GLsizei*,GLchar*);
// CREATEPROGRAMPROC)(void);
// ATTACHSHADERPROC)(GLuint,GLuint);
// LINKPROGRAMPROC)(GLuint);
// GETPROGRAMIVPROC)(GLuint,GLenum,GLint*);
// GETPROGRAMINFOLOGPROC)(GLuint,GLsizei,GLsizei*,GLchar*);
// TEXIMAGE2DPROC)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
// GENTEXTURESPROC)(GLsizei,GLuint*);
// GENBUFFERSPROC)(GLsizei,GLuint*);
// GENVERTEXARRAYSPROC)(GLsizei,GLuint*);
// GENVERTEXARRAYSPROC)(GLsizei, GLuint*);
// DELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
// DELETEBUFFERSPROC)(GLsizei, const GLuint*);
// DELETETEXTURESPROC)(GLsizei, const GLuint*);

// Function pointers
// CLEARPROC glClear;
// CLEARCOLORPROC glClearColor;
// ENABLEPROC glEnable;
// DISABLEPROC glDisable;
// DRAWARRAYSPROC glDrawArrays;
// BINDTEXTUREPROC glBindTexture;
// ACTIVETEXTUREPROC glActiveTexture;
// BINDVERTEXARRAYPROC glBindVertexArray;
// BINDBUFFERPROC glBindBuffer;
// BUFFERDATAPROC glBufferData;
// GETERRORPROC glGetError;
// GETINTEGERVPROC glGetIntegerv;
// GETSTRINGPROC glGetString;
// UNIFORM1IPROC glUniform1i;
// UNIFORM1FPROC glUniform1f;
// UNIFORM2FPROC glUniform2f;
// UNIFORM3FPROC glUniform3f;
// UNIFORM4FPROC glUniform4f;
// GETUNIFORMLOCATIONPROC glGetUniformLocation;
// USEPROGRAMPROC glUseProgram;
// DELETEBUFFERSPROC glDeleteBuffers;
// DELETETEXTURESPROC glDeleteTextures;
// DELETEVERTEXARRAYSPROC glDeleteVertexArrays;
// DELETEPROGRAMPROC glDeleteProgram;
// DELETESHADERPROC glDeleteShader;
// ATTACHSHADERPROC glAttachShader;
// DETACHSHADERPROC glDetachShader;
// LINKPROGRAMPROC glLinkProgram;
// PIXELSTOREIPROC glPixelStorei;
// TEXPARAMETERIPROC glTexParameteri;
// VERTEXATTRIBPOINTERPROC glVertexAttribPointer;
// ENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
// DISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
// CREATESHADERPROC glCreateShader;
static PFNGCSHADERSOURCEPROC glShaderSource;
// COMPILESHADERPROC glCompileShader;
// GETSHADERIVPROC glGetShaderiv;
// GETSHADERINFOLOGPROC glGetShaderInfoLog;
// CREATEPROGRAMPROC glCreateProgram;
// ATTACHSHADERPROC glAttachShader;
// LINKPROGRAMPROC glLinkProgram;
// GETPROGRAMIVPROC glGetProgramiv;
// GETPROGRAMINFOLOGPROC glGetProgramInfoLog;
// TEXIMAGE2DPROC glTexImage2D;
// GENTEXTURESPROC glGenTextures;
// GENBUFFERSPROC glGenBuffers;
// GENVERTEXARRAYSPROC glGenVertexArrays;

static bool gladLoadGLLoader_unused() {
    glClear = (PFNGLCLEARPROC)getGLProcAddress("glClear");
    glClearColor = (PFNGLCLEARCOLORPROC)getGLProcAddress("glClearColor");
    glEnable = (PFNGLENABLEPROC)getGLProcAddress("glEnable");
    glDisable = (PFNGLDISABLEPROC)getGLProcAddress("glDisable");
    glDrawArrays = (PFNGLDRAWARRAYSPROC)getGLProcAddress("glDrawArrays");
    glBindTexture = (PFNGLBINDTEXTUREPROC)getGLProcAddress("glBindTexture");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)getGLProcAddress("glActiveTexture");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)getGLProcAddress("glBindVertexArray");
    glBindBuffer = (PFNGLBINDBUFFERPROC)getGLProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)getGLProcAddress("glBufferData");
    glGetError = (PFNGLGETERRORPROC)getGLProcAddress("glGetError");
    glGetIntegerv = (PFNGLGETINTEGERVPROC)getGLProcAddress("glGetIntegerv");
    glGetString = (PFNGLGETSTRINGPROC)getGLProcAddress("glGetString");
    glUniform1i = (PFNGLUNIFORM1IPROC)getGLProcAddress("glUniform1i");
    glUniform1f = (PFNGLUNIFORM1FPROC)getGLProcAddress("glUniform1f");
    glUniform2f = (PFNGLUNIFORM2FPROC)getGLProcAddress("glUniform2f");
    glUniform3f = (PFNGLUNIFORM3FPROC)getGLProcAddress("glUniform3f");
    glUniform4f = (PFNGLUNIFORM4FPROC)getGLProcAddress("glUniform4f");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)getGLProcAddress("glGetUniformLocation");
    glUseProgram = (PFNGLUSEPROGRAMPROC)getGLProcAddress("glUseProgram");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)getGLProcAddress("glDeleteBuffers");
    glDeleteTextures = (PFNGLDELETETEXTURESPROC)getGLProcAddress("glDeleteTextures");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)getGLProcAddress("glDeleteVertexArrays");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)getGLProcAddress("glDeleteProgram");
    glDeleteShader = (PFNGLDELETESHADERPROC)getGLProcAddress("glDeleteShader");
    glAttachShader = (PFNGLATTACHSHADERPROC)getGLProcAddress("glAttachShader");
    glDetachShader = (PFNGLDETACHSHADERPROC)getGLProcAddress("glDetachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)getGLProcAddress("glLinkProgram");
    glPixelStorei = (PFNGLPIXELSTOREIPROC)getGLProcAddress("glPixelStorei");
    glTexParameteri = (PFNGLTEXPARAMETERIPROC)getGLProcAddress("glTexParameteri");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)getGLProcAddress("glVertexAttribPointer");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)getGLProcAddress("glEnableVertexAttribArray");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)getGLProcAddress("glDisableVertexAttribArray");
    glCreateShader = (PFNGLCREATESHADERPROC)getGLProcAddress("glCreateShader");
    glShaderSource = (PFNGCSHADERSOURCEPROC)getGLProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)getGLProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)getGLProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)getGLProcAddress("glGetShaderInfoLog");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)getGLProcAddress("glCreateProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)getGLProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)getGLProcAddress("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)getGLProcAddress("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)getGLProcAddress("glGetProgramInfoLog");
    glTexImage2D = (PFNGLTEXIMAGE2DPROC)getGLProcAddress("glTexImage2D");
    glGenTextures = (PFNGLGENTEXTURESPROC)getGLProcAddress("glGenTextures");
    glGenBuffers = (PFNGLGENBUFFERSPROC)getGLProcAddress("glGenBuffers");
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)getGLProcAddress("glGenVertexArrays");

    return glClear && glClearColor && glDrawArrays && glBindTexture && glGetError;
}

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

        // Initialize GL loader
        if (!gladLoadGLLoader()) {
            throw std::runtime_error("Failed to initialize OpenGL functions");
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

bool Window::isKeyPressed(int key) const { 
    return glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS; 
}
void Window::display(const float* fb, int w, int h) { 
    if(fb) { glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, fb); }
    glDrawArrays(GL_TRIANGLES, 0, 6); 
    glfwSwapBuffers(glfwGetCurrentContext());
}
void Window::init(int w, int h) {
    // Handled by your actual Impl
}
