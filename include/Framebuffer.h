#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

// Thread-safe CPU framebuffer.
// Stores per-pixel HDR running sum + sample counter.
// addSample is thread-safe only when different threads touch different pixels —
// the renderer guarantees this by partitioning pixels across tiles.
class Framebuffer {
public:
    Framebuffer() = default;

    void resize(int w, int h);
    void reset();     // zero accumulator + counts; call on camera move or seed change.

    int width()  const { return w_; }
    int height() const { return h_; }

    // Add one HDR sample to pixel (x, y). Must only be called by one thread per pixel.
    void addSample(int x, int y, const glm::vec3& color);

    // Build LDR upload buffer (tone-mapped, gamma-corrected) and return pointer.
    // Lazy: recomputed each call. 3 floats/pixel in [0,1], row 0 = top.
    const float* uploadPixels();

    // Save the current image as a PNG via stb_image_write. Returns true on success.
    bool savePNG(const char* path);

    // Total samples dispatched across all pixels (rough progress meter).
    int minSampleCount() const;

private:
    int w_ = 0, h_ = 0;
    std::vector<glm::vec3> accum_;     // HDR sum
    std::vector<int>       counts_;    // samples per pixel
    std::vector<float>     upload_;    // 3 floats per pixel, tone-mapped

    // ACES filmic tone map per channel.
    static inline float acesTonemap(float x) {
        const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
        float num = x * (a * x + b);
        float den = x * (c * x + d) + e;
        float y = num / den;
        return y < 0.0f ? 0.0f : (y > 1.0f ? 1.0f : y);
    }
};
