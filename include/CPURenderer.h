#pragma once

#include "IRenderer.h"
#include "Renderer.h"
#include "Framebuffer.h"

class Camera;
namespace RayTrace { struct Context; }

// Adapter: wraps the existing tile-based CPU Renderer + Framebuffer so main.cpp
// can treat it as an IRenderer and swap it for the GPURenderer at startup.
class CPURenderer : public IRenderer {
public:
    CPURenderer() = default;

    bool init(int w, int h, Camera* cam, const RayTrace::Context& ctx,
              int tileSize = 32, int threadCount = 0);

    void renderPass(int spp) override    { inner_.renderTiles(spp); }
    void reset() override                { fb_.reset(); }
    int  width()  const override         { return fb_.width();  }
    int  height() const override         { return fb_.height(); }
    void resize(int w, int h) override   { fb_.resize(w, h); }
    bool savePNG(const char* path) override { return fb_.savePNG(path); }
    int  samplesAccumulated() const override { return fb_.minSampleCount(); }
    const char* backendName() const override { return "CPU (multithreaded)"; }

    // The window needs the CPU framebuffer for its existing blit() path.
    Framebuffer& framebuffer() { return fb_; }

private:
    Framebuffer fb_;
    Renderer    inner_;
};
