#pragma once

#include <atomic>
#include "RayTrace.h"

class Framebuffer;
class Camera;

class Renderer {
public:
    Renderer() = default;

    void init(Framebuffer* fb,
              Camera*      cam,
              const RayTrace::Context& ctx,
              int tileSize      = 32,
              int threadCount   = 0 /* 0 = hardware_concurrency */);

    // Add `spp` samples to every pixel, in parallel across tiles.
    void renderTiles(int spp);

    int tileSize()    const { return tileSize_; }
    int threadCount() const { return threads_; }

    // Rough progress (fraction of tiles done in last dispatch).
    float lastProgress() const { return lastProgress_.load(); }

private:
    Framebuffer* fb_  = nullptr;
    Camera*      cam_ = nullptr;
    RayTrace::Context ctx_{};
    int tileSize_ = 32;
    int threads_  = 1;

    std::atomic<float> lastProgress_{0.0f};
};
