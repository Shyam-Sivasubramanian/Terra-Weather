#include "Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <cmath>
#include <cstdio>
#include <algorithm>

bool Texture2D::load(const std::string& path) {
    int w, h, n;
    stbi_set_flip_vertically_on_load(0); // row 0 = top to match our convention
    unsigned char* px = stbi_load(path.c_str(), &w, &h, &n, 3);
    if (!px) {
        std::fprintf(stderr, "Texture2D: failed to load '%s' (%s)\n",
                     path.c_str(), stbi_failure_reason());
        w_ = h_ = 0;
        data_.clear();
        return false;
    }
    w_ = w; h_ = h;
    const std::size_t total = static_cast<std::size_t>(w) * h * 3;
    data_.resize(total);
    // Assume sRGB input; convert to linear so lighting math is correct.
    for (std::size_t i = 0; i < total; ++i) {
        float s = px[i] / 255.0f;
        // Fast srgb->linear: pow approximation
        data_[i] = std::pow(s, 2.2f);
    }
    stbi_image_free(px);
    return true;
}

glm::vec3 Texture2D::sample(float u, float v) const {
    if (data_.empty() || w_ <= 0 || h_ <= 0) return fallback_;

    // Wrap
    u = u - std::floor(u);
    v = v - std::floor(v);
    // Convert to pixel-space continuous coords.
    float fx = u * (w_ - 1);
    float fy = v * (h_ - 1);
    int x0 = static_cast<int>(std::floor(fx));
    int y0 = static_cast<int>(std::floor(fy));
    int x1 = std::min(x0 + 1, w_ - 1);
    int y1 = std::min(y0 + 1, h_ - 1);
    float tx = fx - x0;
    float ty = fy - y0;
    x0 = std::clamp(x0, 0, w_ - 1);
    y0 = std::clamp(y0, 0, h_ - 1);

    auto fetch = [&](int x, int y) {
        std::size_t i = (static_cast<std::size_t>(y) * w_ + x) * 3;
        return glm::vec3(data_[i], data_[i + 1], data_[i + 2]);
    };
    glm::vec3 c00 = fetch(x0, y0);
    glm::vec3 c10 = fetch(x1, y0);
    glm::vec3 c01 = fetch(x0, y1);
    glm::vec3 c11 = fetch(x1, y1);
    glm::vec3 a = glm::mix(c00, c10, tx);
    glm::vec3 b = glm::mix(c01, c11, tx);
    return glm::mix(a, b, ty);
}
