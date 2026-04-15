#pragma once

#include "glm/glm.hpp"
#include <memory>

/**
 * @brief Base class for texture mapping
 */
class Texture {
public:
    virtual ~Texture() = default;

    /**
     * @brief Get color at texture coordinates
     * @param u U coordinate [0, 1]
     * @param v V coordinate [0, 1]
     * @param p World position (for procedural textures)
     */
    virtual glm::vec3 value(float u, float v, const glm::vec3& p) const = 0;
};

/**
 * @brief Solid color texture
 */
class SolidColor : public Texture {
public:
    glm::vec3 color;

    SolidColor(const glm::vec3& c) : color(c) {}
    SolidColor(float r, float g, float b) : color(r, g, b) {}

    glm::vec3 value(float u, float v, const glm::vec3& p) const override {
        return color;
    }
};

/**
 * @brief Checkerboard texture pattern
 */
class CheckerTexture : public Texture {
public:
    std::shared_ptr<Texture> odd;
    std::shared_ptr<Texture> even;
    float scale;

    CheckerTexture(float s, std::shared_ptr<Texture> o, std::shared_ptr<Texture> e)
        : scale(s), odd(o), even(e) {}

    CheckerTexture(const glm::vec3& c1, const glm::vec3& c2, float s = 1.0f)
        : scale(s), odd(std::make_shared<SolidColor>(c1)),
          even(std::make_shared<SolidColor>(c2)) {}

    glm::vec3 value(float u, float v, const glm::vec3& p) const override {
        float s = scale;
        float x = floor(s * p.x);
        float y = floor(s * p.y);
        float z = floor(s * p.z);

        float sum = x + y + z;
        bool isEven = (int(sum) % 2) == 0;

        return isEven ? even->value(u, v, p) : odd->value(u, v, p);
    }
};

/**
 * @brief Noise-based texture
 */
class NoiseTexture : public Texture {
public:
    float scale;
    float turbulence;
    int octaves;

    NoiseTexture(float s = 1.0f, float turb = 0.0f, int oct = 6)
        : scale(s), turbulence(turb), octaves(oct) {}

    glm::vec3 value(float u, float v, const glm::vec3& p) const override {
        // Perlin noise with turbulence
        float noise = 0.5f + 0.5f * noiseFunc(scale * p);

        if (turbulence > 0.0f) {
            noise += turbulence * turbulenceFunc(scale * p, octaves);
        }

        return glm::vec3(noise);
    }

private:
    float noiseFunc(const glm::vec3& p) const;
    float turbulenceFunc(const glm::vec3& p, int oct) const;
};

/**
 * @brief Image texture
 */
class ImageTexture : public Texture {
public:
    int width = 0;
    int height = 0;
    std::vector<unsigned char> data;

    ImageTexture() {}
    ImageTexture(const std::string& filename);

    bool load(const std::string& filename);

    glm::vec3 value(float u, float v, const glm::vec3& p) const override {
        if (width == 0 || height == 0) return glm::vec3(0.0f);

        // Flip V coordinate (image convention vs texture convention)
        int i = static_cast<int>(u * width);
        int j = static_cast<int>((1.0f - v) * height);

        i = std::clamp(i, 0, width - 1);
        j = std::clamp(j, 0, height - 1);

        int idx = (j * width + i) * 3;
        return glm::vec3(data[idx], data[idx + 1], data[idx + 2]) / 255.0f;
    }
};
