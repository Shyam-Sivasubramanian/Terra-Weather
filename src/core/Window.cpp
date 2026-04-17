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
typedef size_t GLsizeiptr;
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

typedef void (APIENTRYP PFNGLCLEARPROC)(GLbitfield);
typedef void (APIENTRYP PFNGLCLEARCOLORPROC)(GLfloat,GLfloat,GLfloat,GLfloat);
typedef void (APIENTRYP PFNGLENABLEPROC)(GLenum);
typedef void (APIENTRYP PFNGLDISABLEPROC)(GLenum);
typedef void (APIENTRYP PFNGLDRAWARRAYSPROC)(GLenum,GLint,GLsizei);
typedef void (APIENTRYP PFNGLBINDTEXTUREPROC)(GLenum,GLuint);
typedef void (APIENTRYP PFNGLACTIVETEXTUREPROC)(GLenum);
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint);
typedef void (APIENTRYP PFNGLBINDBUFFERPROC)(GLenum,GLuint);
typedef void (APIENTRYP PFNGLBUFFERDATAPROC)(GLenum,GLsizeiptr,const void*,GLenum);
typedef void (APIENTRYP PFNGLDRAWRANGEELEMENTSPROC)(GLenum,GLuint,GLuint,GLsizei,GLenum,const void*);
typedef GLenum (APIENTRYP PFNGLGETERRORPROC)(void);
typedef void (APIENTRYP PFNGLGETINTEGERVPROC)(GLenum,GLint*);
typedef const GLubyte* (APIENTRYP PFNGLGETSTRINGPROC)(GLenum);
typedef void (APIENTRYP PFNGLUNIFORM1IPROC)(GLint,GLint);
typedef void (APIENTRYP PFNGLUNIFORM1FPROC)(GLint,GLfloat);
typedef void (APIENTRYP PFNGLUNIFORM2FPROC)(GLint,GLfloat,GLfloat);
typedef void (APIENTRYP PFNGLUNIFORM3FPROC)(GLint,GLfloat,GLfloat,GLfloat);
typedef void (APIENTRYP PFNGLUNIFORM4FPROC)(GLint,GLfloat,GLfloat,GLfloat,GLfloat);
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC)(GLuint,const GLchar*);
typedef void (APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC)(GLsizei,const GLuint*);
typedef void (APIENTRYP PFNGLDELETETEXTURESPROC)(GLsizei,const GLuint*);
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei,const GLuint*);
typedef void (APIENTRYP PFNGLDELETEPROGRAMPROC)(GLuint);
typedef void (APIENTRYP PFNGLDELETESHADERPROC)(GLuint);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC)(GLuint,GLuint);
typedef void (APIENTRYP PFNGLDETACHSHADERPROC)(GLuint,GLuint);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRYP PFNGLPIXELSTOREIPROC)(GLenum,GLint);
typedef void (APIENTRYP PFNGLTEXPARAMETERIPROC)(GLenum,GLenum,GLint);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERPROC)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYPROC)(GLuint);
typedef void (APIENTRYP PFNGLCREATESHADERPROC)(GLenum);
typedef void (APIENTRYP PFNGCSHADERSOURCEPROC)(GLuint,GLsizei,const GLchar*const*,const GLint*);
typedef void (APIENTRYP PFNGLCOMPILESHADERPROC)(GLuint);
typedef void (APIENTRYP PFNGLGETSHADERIVPROC)(GLuint,GLenum,GLint*);
typedef void (APIENTRYP PFNGLGETSHADERINFOLOGPROC)(GLuint,GLsizei,GLsizei*,GLchar*);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRYP PFNGLATTACHSHADERPROC)(GLuint,GLuint);
typedef void (APIENTRYP PFNGLLINKPROGRAMPROC)(GLuint);
typedef void (APIENTRYP PFNGLGETPROGRAMIVPROC)(GLuint,GLenum,GLint*);
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC)(GLuint,GLsizei,GLsizei*,GLchar*);
typedef void (APIENTRYP PFNGLTEXIMAGE2DPROC)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
typedef void (APIENTRYP PFNGLGENTEXTURESPROC)(GLsizei,GLuint*);
typedef void (APIENTRYP PFNGLGENBUFFERSPROC)(GLsizei,GLuint*);
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei,GLuint*);
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSPROC)(GLsizei, GLuint*);
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRYP PFNGLDELETEBUFFERSPROC)(GLsizei, const GLuint*);
typedef void (APIENTRYP PFNGLDELETETEXTURESPROC)(GLsizei, const GLuint*);

// Function pointers
static PFNGLCLEARPROC glClear;
static PFNGLCLEARCOLORPROC glClearColor;
static PFNGLENABLEPROC glEnable;
static PFNGLDISABLEPROC glDisable;
static PFNGLDRAWARRAYSPROC glDrawArrays;
static PFNGLBINDTEXTUREPROC glBindTexture;
static PFNGLACTIVETEXTUREPROC glActiveTexture;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
static PFNGLBINDBUFFERPROC glBindBuffer;
static PFNGLBUFFERDATAPROC glBufferData;
static PFNGLGETERRORPROC glGetError;
static PFNGLGETINTEGERVPROC glGetIntegerv;
static PFNGLGETSTRINGPROC glGetString;
static PFNGLUNIFORM1IPROC glUniform1i;
static PFNGLUNIFORM1FPROC glUniform1f;
static PFNGLUNIFORM2FPROC glUniform2f;
static PFNGLUNIFORM3FPROC glUniform3f;
static PFNGLUNIFORM4FPROC glUniform4f;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
static PFNGLUSEPROGRAMPROC glUseProgram;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers;
static PFNGLDELETETEXTURESPROC glDeleteTextures;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
static PFNGLDELETEPROGRAMPROC glDeleteProgram;
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLDETACHSHADERPROC glDetachShader;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLPIXELSTOREIPROC glPixelStorei;
static PFNGLTEXPARAMETERIPROC glTexParameteri;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGCSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLGETSHADERIVPROC glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
static PFNGLCREATEPROGRAMPROC glCreateProgram;
static PFNGLATTACHSHADERPROC glAttachShader;
static PFNGLLINKPROGRAMPROC glLinkProgram;
static PFNGLGETPROGRAMIVPROC glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
static PFNGLTEXIMAGE2DPROC glTexImage2D;
static PFNGLGENTEXTURESPROC glGenTextures;
static PFNGLGENBUFFERSPROC glGenBuffers;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;

static bool gladLoadGLLoader() {
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
