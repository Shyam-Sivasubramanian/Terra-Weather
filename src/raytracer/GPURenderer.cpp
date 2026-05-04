#include "GPURenderer.h"
#include "WorldData.h"
#include "Camera.h"
#include "Atmosphere.h"
#include "GpuBVHBuilder.h"
#include "GpuSceneData.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <cmath>

#define STBI_WRITE_NO_STDIO_ALREADY_DEFINED_CHECK
// stb_image_write is already implementation-included in Framebuffer.cpp; only
// declarations here.
extern "C" int stbi_write_png(const char* filename, int w, int h, int comp,
                              const void* data, int stride_bytes);

GPURenderer::~GPURenderer() {
    if (accumTex_)    glDeleteTextures(1, &accumTex_);
    if (bvhSSBO_)     glDeleteBuffers(1, &bvhSSBO_);
    if (triSSBO_)     glDeleteBuffers(1, &triSSBO_);
    if (climateSSBO_) glDeleteBuffers(1, &climateSSBO_);
    if (cloudSSBO_)   glDeleteBuffers(1, &cloudSSBO_);
    if (quadVBO_)     glDeleteBuffers(1, &quadVBO_);
    if (quadVAO_)     glDeleteVertexArrays(1, &quadVAO_);
    if (computeProg_) glDeleteProgram(computeProg_);
    if (displayProg_) glDeleteProgram(displayProg_);
}

bool GPURenderer::checkCapabilities() const {
    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    int version = major * 100 + minor * 10;
    if (version < 430) {
        std::fprintf(stderr,
            "[GPURenderer] OpenGL %d.%d < 4.3, compute shaders unavailable.\n",
            major, minor);
        return false;
    }
    return true;
}

std::string GPURenderer::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

unsigned int GPURenderer::compileShader(unsigned int type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* p = src.c_str();
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[GPURenderer] shader compile failed:\n%s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

unsigned int GPURenderer::linkProgram(std::vector<unsigned int> shaders) {
    GLuint p = glCreateProgram();
    for (auto s : shaders) glAttachShader(p, s);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[4096];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[GPURenderer] program link failed:\n%s\n", log);
        glDeleteProgram(p);
        for (auto s : shaders) glDeleteShader(s);
        return 0;
    }
    for (auto s : shaders) glDeleteShader(s);
    return p;
}

bool GPURenderer::loadAndCompilePrograms() {
    // Compute shader.
    std::string cs = readFile("shaders/raytrace.comp");
    if (cs.empty()) {
        std::fprintf(stderr, "[GPURenderer] missing shaders/raytrace.comp\n");
        return false;
    }
    GLuint csh = compileShader(GL_COMPUTE_SHADER, cs);
    if (!csh) return false;
    computeProg_ = linkProgram({csh});
    if (!computeProg_) return false;

    // Display shaders.
    std::string vs = readFile("shaders/display.vert");
    std::string fs = readFile("shaders/display.frag");
    if (vs.empty() || fs.empty()) {
        std::fprintf(stderr, "[GPURenderer] missing display shaders\n");
        return false;
    }
    GLuint vsh = compileShader(GL_VERTEX_SHADER, vs);
    GLuint fsh = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!vsh || !fsh) return false;
    displayProg_ = linkProgram({vsh, fsh});
    if (!displayProg_) return false;

    // Cache uniform locations.
    auto U = [&](const char* name) { return glGetUniformLocation(computeProg_, name); };
    locCamPos_       = U("uCamPos");
    locCamFwd_       = U("uCamForward");
    locCamRight_     = U("uCamRight");
    locCamUp_        = U("uCamUp");
    locTanHalfFov_   = U("uTanHalfFov");
    locAspect_       = U("uAspect");
    locResolution_   = U("uResolution");
    locFrameIndex_   = U("uFrameIndex");
    locMaxDepth_     = U("uMaxDepth");
    locSunDir_       = U("uSunDir");
    locSunColor_     = U("uSunColor");
    locDirectSun_    = U("uDirectSun");
    locWidth_        = U("uWidth");
    locHeight_       = U("uHeight");
    locCloudLayers_  = U("uCloudLayers");
    locWorldScale_   = U("uWorldScale");
    locHeightScale_  = U("uHeightScale");
    locCloudAltMin_  = U("uCloudAltMin");
    locCloudAltMax_  = U("uCloudAltMax");
    locSnowLevel_    = U("uSnowLevel");
    locSeaLevel_     = U("uSeaLevel");
    locGrassTint_    = U("uGrassTint");
    locRockTint_     = U("uRockTint");
    locSnowTintU_    = U("uSnowTint");
    locDebugWeather_ = U("uDebugWeather");

    locDisplayAccum_ = glGetUniformLocation(displayProg_, "uAccum");
    return true;
}

void GPURenderer::createAccumTex(int w, int h) {
    if (accumTex_) glDeleteTextures(1, &accumTex_);
    glGenTextures(1, &accumTex_);
    glBindTexture(GL_TEXTURE_2D, accumTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // Clear to zero.
    std::vector<float> zeros(static_cast<std::size_t>(w) * h * 4, 0.0f);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_FLOAT, zeros.data());
}

void GPURenderer::createQuad() {
    float verts[] = {
        // pos      // uv
        -1.f, -1.f,  0.f, 0.f,
         1.f, -1.f,  1.f, 0.f,
         1.f,  1.f,  1.f, 1.f,
        -1.f, -1.f,  0.f, 0.f,
         1.f,  1.f,  1.f, 1.f,
        -1.f,  1.f,  0.f, 1.f,
    };
    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

bool GPURenderer::init(int w, int h, Camera* cam,
                       const WorldData* world, const Atmosphere* atmosphere) {
    if (!checkCapabilities()) return false;
    cam_ = cam;
    world_ = world;
    atmosphere_ = atmosphere;
    width_ = w; height_ = h;

    if (!loadAndCompilePrograms()) return false;

    glGenBuffers(1, &bvhSSBO_);
    glGenBuffers(1, &triSSBO_);
    glGenBuffers(1, &climateSSBO_);
    glGenBuffers(1, &cloudSSBO_);
    createAccumTex(w, h);
    createQuad();

    if (world_) uploadScene(*world_);
    ready_ = true;
    std::fprintf(stderr, "[GPURenderer] initialized %dx%d\n", w, h);
    return true;
}

void GPURenderer::uploadScene(const WorldData& world) {
    GpuSceneData data = GpuBVHBuilder::build(world);
    numBVHNodes_ = static_cast<int>(data.nodes.size());
    numTris_     = static_cast<int>(data.tris.size());

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, bvhSSBO_);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 data.nodes.size() * sizeof(GpuBVHNode),
                 data.nodes.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triSSBO_);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 data.tris.size() * sizeof(GpuTri),
                 data.tris.data(), GL_STATIC_DRAW);

    uploadClimate(world);
}

void GPURenderer::uploadClimate(const WorldData& world) {
    // Pack humidity + weather into one SSBO (back-to-back).
    const std::size_t n = world.humidityMap.size();
    std::vector<float> climate(n * 2, 0.0f);
    std::copy(world.humidityMap.begin(), world.humidityMap.end(), climate.begin());
    std::copy(world.weatherMap.begin(),  world.weatherMap.end(),  climate.begin() + n);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, climateSSBO_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, climate.size() * sizeof(float),
                 climate.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, cloudSSBO_);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 world.cloudDensity.size() * sizeof(float),
                 world.cloudDensity.data(), GL_DYNAMIC_DRAW);
}

void GPURenderer::reset() {
    accumulatedSamples_ = 0;
    frameIndex_ = 0;
    // Clear accumulation texture to zero.
    std::vector<float> zeros(static_cast<std::size_t>(width_) * height_ * 4, 0.0f);
    glBindTexture(GL_TEXTURE_2D, accumTex_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_,
                    GL_RGBA, GL_FLOAT, zeros.data());
}

void GPURenderer::bindAllBuffers() {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bvhSSBO_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, triSSBO_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, climateSSBO_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, cloudSSBO_);
    glBindImageTexture(0, accumTex_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

void GPURenderer::pushUniforms() {
    glm::vec3 fwd   = cam_->forward();
    glm::vec3 right = cam_->right();
    glm::vec3 up    = cam_->viewUp();
    float aspect      = static_cast<float>(width_) / std::max(1, height_);
    float tanHalfFov  = std::tan(glm::radians(cam_->vfovDeg()) * 0.5f);

    glUseProgram(computeProg_);
    glUniform3fv(locCamPos_,   1, &cam_->position().x);
    glUniform3fv(locCamFwd_,   1, &fwd.x);
    glUniform3fv(locCamRight_, 1, &right.x);
    glUniform3fv(locCamUp_,    1, &up.x);
    glUniform1f(locTanHalfFov_, tanHalfFov);
    glUniform1f(locAspect_,     aspect);
    glUniform2f(locResolution_, static_cast<float>(width_),
                                static_cast<float>(height_));
    glUniform1i(locFrameIndex_, frameIndex_);
    glUniform1i(locMaxDepth_,   4);
    glUniform1i(locDirectSun_,  1);

    if (atmosphere_) {
        glm::vec3 sd = atmosphere_->getSunDirection();
        glm::vec3 sc = atmosphere_->getSunColor();
        glUniform3fv(locSunDir_,   1, &sd.x);
        glUniform3fv(locSunColor_, 1, &sc.x);
    } else {
        float d[3] = {0, 1, 0}; float c[3] = {1, 1, 1};
        glUniform3fv(locSunDir_,   1, d);
        glUniform3fv(locSunColor_, 1, c);
    }

    if (world_) {
        glUniform1i(locWidth_,       world_->width);
        glUniform1i(locHeight_,      world_->height);
        glUniform1i(locCloudLayers_, world_->cloudLayers);
        glUniform1f(locWorldScale_,  world_->worldScale);
        glUniform1f(locHeightScale_, world_->heightScale);
        glUniform1f(locCloudAltMin_, world_->cloudAltMin);
        glUniform1f(locCloudAltMax_, world_->cloudAltMax);
        glUniform1f(locSnowLevel_,   world_->snowLevel * world_->heightScale);
        glUniform1f(locSeaLevel_,    world_->seaLevel  * world_->heightScale);
    }
    const float grass[3] = {0.18f, 0.35f, 0.10f};   // deeper forest green
    const float rock [3] = {0.28f, 0.24f, 0.22f};   // darker slate rock
    const float snow [3] = {0.90f, 0.92f, 0.96f};   // slightly off-white snow
    glUniform3fv(locGrassTint_,  1, grass);
    glUniform3fv(locRockTint_,   1, rock);
    glUniform3fv(locSnowTintU_,  1, snow);
    glUniform1i(locDebugWeather_, debugWeather_ ? 1 : 0);
}

void GPURenderer::renderPass(int spp) {
    if (!ready_) return;
    for (int s = 0; s < spp; ++s) {
        pushUniforms();
        bindAllBuffers();
        int gx = (width_  + 15) / 16;
        int gy = (height_ + 15) / 16;
        glDispatchCompute(gx, gy, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT |
                        GL_TEXTURE_FETCH_BARRIER_BIT);
        ++frameIndex_;
        ++accumulatedSamples_;
    }
}

void GPURenderer::drawDirect(int winW, int winH) {
    if (!ready_) return;
    glViewport(0, 0, winW, winH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(displayProg_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, accumTex_);
    if (locDisplayAccum_ >= 0) glUniform1i(locDisplayAccum_, 0);
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GPURenderer::resize(int w, int h) {
    width_ = w; height_ = h;
    createAccumTex(w, h);
    reset();
}

bool GPURenderer::savePNG(const char* path) {
    // Read back the accumulation texture, divide by count, tone-map, write PNG.
    std::vector<float> buf(static_cast<std::size_t>(width_) * height_ * 4);
    glBindTexture(GL_TEXTURE_2D, accumTex_);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, buf.data());

    std::vector<unsigned char> out(static_cast<std::size_t>(width_) * height_ * 3);
    auto aces = [](float x) {
        float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
        float y = (x * (a * x + b)) / (x * (c * x + d) + e);
        return std::clamp(y, 0.0f, 1.0f);
    };
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            std::size_t si = (static_cast<std::size_t>(y) * width_ + x) * 4;
            float count = buf[si + 3];
            float r = count > 0 ? buf[si + 0] / count : 0.0f;
            float g = count > 0 ? buf[si + 1] / count : 0.0f;
            float b = count > 0 ? buf[si + 2] / count : 0.0f;
            r = std::pow(aces(r), 1.0f / 2.2f);
            g = std::pow(aces(g), 1.0f / 2.2f);
            b = std::pow(aces(b), 1.0f / 2.2f);
            // Compute shader stored row 0 at top of image (pix.y = 0 means top).
            // glGetTexImage returns texels in storage order: row 0 first.
            // stb_image_write also treats row 0 as top. No flip needed.
            std::size_t di = (static_cast<std::size_t>(y) * width_ + x) * 3;
            out[di + 0] = static_cast<unsigned char>(r * 255.0f + 0.5f);
            out[di + 1] = static_cast<unsigned char>(g * 255.0f + 0.5f);
            out[di + 2] = static_cast<unsigned char>(b * 255.0f + 0.5f);
        }
    }
    return stbi_write_png(path, width_, height_, 3, out.data(), width_ * 3) != 0;
}
