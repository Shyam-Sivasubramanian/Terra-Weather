#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

// Simple RGB float texture. Loads via stb_image; if loading fails, stays empty
// and sample() returns a fallback color so the renderer keeps working without
// assets on disk.
class Texture2D {
public:
    Texture2D() = default;

    bool load(const std::string& path);
    void setFallback(const glm::vec3& c) { fallback_ = c; }
    bool loaded() const { return !data_.empty(); }

    // u, v in [0,1]; wraps via repeat. Bilinear.
    glm::vec3 sample(float u, float v) const;

private:
    int w_ = 0, h_ = 0;
    std::vector<float> data_; // RGB, row 0 at top
    glm::vec3 fallback_{0.7f, 0.7f, 0.7f};
};
