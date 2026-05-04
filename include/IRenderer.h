#pragma once

// Common interface for CPU path tracer (Renderer) and GPU compute-shader
// path tracer (GPURenderer). main.cpp drives either one through this.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    // Run one accumulation pass. spp = samples per pixel added by this call.
    virtual void renderPass(int spp) = 0;

    // Invoked when camera moves or seed changes — clear accumulator.
    virtual void reset() = 0;

    // Produce pixels for the Window to blit. For the CPU renderer this is a
    // CPU buffer; for the GPU renderer this returns nullptr and the
    // renderer emits pixels directly into a texture the window samples.
    // Window::blit() calls drawDirect() if useDirect() returns true.
    virtual bool useDirect() const { return false; }
    virtual void drawDirect(int winW, int winH) {}

    // Current render-target resolution (so main can auto-resize on window change).
    virtual int width()  const = 0;
    virtual int height() const = 0;

    virtual void resize(int w, int h) = 0;

    // Save current accumulation to a PNG file.
    virtual bool savePNG(const char* path) = 0;

    // For status readout.
    virtual int  samplesAccumulated() const { return 0; }
    virtual const char* backendName() const = 0;
};
