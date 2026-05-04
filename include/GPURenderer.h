#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "IRenderer.h"

struct WorldData;
class Camera;
class Atmosphere;

// GPU path-traced renderer using an OpenGL 4.3 compute shader.
// Owns SSBOs for flattened BVH + triangles + climate data, and a single
// rgba32f accumulation texture that's both compute-shader output and the
// display shader's sampled texture.
class GPURenderer : public IRenderer {
public:
    GPURenderer() = default;
    ~GPURenderer() override;

    // Returns false if compute shaders / GL 4.3 features aren't available.
    // Caller should fall back to the CPU renderer in that case.
    bool init(int w, int h, Camera* cam, const WorldData* world,
              const Atmosphere* atmosphere);

    // Rebuild GPU scene data from WorldData. Call at startup, on seed change,
    // and whenever climate fields (humidity, weather, cloud density) update.
    void uploadScene(const WorldData& world);
    // Lighter upload: just the per-frame climate fields. Use if geometry is
    // unchanged but weather/clouds were re-simulated.
    void uploadClimate(const WorldData& world);

    void renderPass(int spp) override;
    void reset() override;

    bool useDirect() const override { return true; }
    void drawDirect(int winW, int winH) override;

    int  width()  const override { return width_; }
    int  height() const override { return height_; }
    void resize(int w, int h) override;

    bool savePNG(const char* path) override;
    int  samplesAccumulated() const override { return accumulatedSamples_; }
    const char* backendName() const override { return "GPU (compute shader)"; }

    // Expose so main.cpp can know whether to use the CPU blit path.
    bool ready() const { return ready_; }

    // Toggle weather debug overlay. When on, terrain is tinted by weather
    // cell classification (rain=blue, snow=cyan-white).
    void setDebugWeather(bool on) { debugWeather_ = on; }
    bool debugWeather() const { return debugWeather_; }

private:
    // GL objects.
    unsigned int computeProg_  = 0;
    unsigned int displayProg_  = 0;
    unsigned int accumTex_     = 0;
    unsigned int bvhSSBO_      = 0;
    unsigned int triSSBO_      = 0;
    unsigned int climateSSBO_  = 0;
    unsigned int cloudSSBO_    = 0;
    unsigned int quadVAO_      = 0;
    unsigned int quadVBO_      = 0;

    // Cached uniform locations (compute program).
    int locCamPos_, locCamFwd_, locCamRight_, locCamUp_;
    int locTanHalfFov_, locAspect_, locResolution_, locFrameIndex_, locMaxDepth_;
    int locSunDir_, locSunColor_, locDirectSun_;
    int locWidth_, locHeight_, locCloudLayers_;
    int locWorldScale_, locHeightScale_;
    int locCloudAltMin_, locCloudAltMax_;
    int locSnowLevel_, locSeaLevel_;
    int locGrassTint_, locRockTint_, locSnowTintU_;
    int locDebugWeather_;
    int locDisplayAccum_;

    Camera*           cam_        = nullptr;
    const WorldData*  world_      = nullptr;
    const Atmosphere* atmosphere_ = nullptr;

    int width_  = 0;
    int height_ = 0;
    int accumulatedSamples_ = 0;
    int frameIndex_         = 0;

    int numBVHNodes_ = 0;
    int numTris_     = 0;

    bool ready_ = false;
    bool debugWeather_ = false;

    bool checkCapabilities() const;
    bool loadAndCompilePrograms();
    void createAccumTex(int w, int h);
    void createQuad();

    // Helpers
    static std::string readFile(const std::string& path);
    static unsigned int compileShader(unsigned int type, const std::string& src);
    static unsigned int linkProgram(std::vector<unsigned int> shaders);

    void bindAllBuffers();
    void pushUniforms();
};
