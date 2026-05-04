#include "Framebuffer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void Framebuffer::resize(int w, int h) {
    w_ = w; h_ = h;
    const std::size_t n = static_cast<std::size_t>(w) * h;
    accum_.assign(n, glm::vec3(0.0f));
    counts_.assign(n, 0);
    upload_.assign(n * 3, 0.0f);
}

void Framebuffer::reset() {
    std::fill(accum_.begin(),  accum_.end(),  glm::vec3(0.0f));
    std::fill(counts_.begin(), counts_.end(), 0);
    std::fill(upload_.begin(), upload_.end(), 0.0f);
}

void Framebuffer::addSample(int x, int y, const glm::vec3& color) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    // Guard against NaN / inf from divergent bounces.
    glm::vec3 c = color;
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(c[i]) || c[i] < 0.0f) c[i] = 0.0f;
    }
    std::size_t idx = static_cast<std::size_t>(y) * w_ + x;
    accum_[idx]  += c;
    counts_[idx] += 1;
}

const float* Framebuffer::uploadPixels() {
    // Convert accumulator to tone-mapped LDR sRGB.
    const std::size_t n = static_cast<std::size_t>(w_) * h_;
    for (std::size_t i = 0; i < n; ++i) {
        int c = counts_[i];
        glm::vec3 col = (c > 0) ? accum_[i] / static_cast<float>(c) : glm::vec3(0.0f);
        // Tone map.
        col.r = acesTonemap(col.r);
        col.g = acesTonemap(col.g);
        col.b = acesTonemap(col.b);
        // Gamma correct (approx sRGB).
        const float invG = 1.0f / 2.2f;
        col.r = std::pow(col.r, invG);
        col.g = std::pow(col.g, invG);
        col.b = std::pow(col.b, invG);
        upload_[i * 3 + 0] = col.r;
        upload_[i * 3 + 1] = col.g;
        upload_[i * 3 + 2] = col.b;
    }
    return upload_.data();
}

bool Framebuffer::savePNG(const char* path) {
    const float* src = uploadPixels();
    std::vector<unsigned char> bytes(static_cast<std::size_t>(w_) * h_ * 3);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        float v = std::clamp(src[i], 0.0f, 1.0f);
        bytes[i] = static_cast<unsigned char>(v * 255.0f + 0.5f);
    }
    // stb_image_write expects row 0 = top; our row 0 IS top.
    return stbi_write_png(path, w_, h_, 3, bytes.data(), w_ * 3) != 0;
}

int Framebuffer::minSampleCount() const {
    if (counts_.empty()) return 0;
    int m = std::numeric_limits<int>::max();
    for (int c : counts_) m = std::min(m, c);
    return m;
}
