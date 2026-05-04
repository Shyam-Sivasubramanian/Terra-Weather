#include "Material.h"
#include "Texture2D.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// ----------------------------------------------------------------------------
// Lambertian diffuse material with constant albedo (used as fallback / for
// testing). Scatters into a cosine-weighted hemisphere.
// ----------------------------------------------------------------------------
class LambertianMaterial : public Material {
public:
    glm::vec3 albedo;
    explicit LambertianMaterial(const glm::vec3& a) : albedo(a) {}

    bool scatter(const Ray& /*in*/, const HitRecord& rec,
                 glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 dir = rec.normal + randomUnitVector();
        if (nearZero(dir)) dir = rec.normal;
        scattered   = Ray(rec.point, glm::normalize(dir));
        attenuation = albedo;
        return true;
    }
};

// ----------------------------------------------------------------------------
// Reflective material (water surface). Mirror reflection plus small
// roughness perturbation so the reflection is not razor-sharp.
// ----------------------------------------------------------------------------
class ReflectiveMaterial : public Material {
public:
    glm::vec3 albedo;
    float     roughness;
    ReflectiveMaterial(const glm::vec3& a, float r)
        : albedo(a), roughness(std::clamp(r, 0.0f, 1.0f)) {}

    bool scatter(const Ray& in, const HitRecord& rec,
                 glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 reflected = glm::reflect(glm::normalize(in.direction), rec.normal);
        glm::vec3 dir = reflected + roughness * randomInUnitSphere();
        scattered   = Ray(rec.point, glm::normalize(dir));
        attenuation = albedo;
        return glm::dot(scattered.direction, rec.normal) > 0.0f;
    }
};

// ----------------------------------------------------------------------------
// Terrain material: blends grass / rock / snow based on height and slope.
// Textures optional — falls back to solid colours when not loaded.
// ----------------------------------------------------------------------------
class TerrainMaterial : public Material {
public:
    const Texture2D* grass = nullptr;
    const Texture2D* rock  = nullptr;
    const Texture2D* snow  = nullptr;

    float snowLevel = 14.0f; // world-space Y threshold (seaLevel..snowLevel*heightScale)
    float heightScale = 20.0f;

    // Tint colours used when a texture is missing.
    glm::vec3 grassTint{0.30f, 0.55f, 0.20f};
    glm::vec3 rockTint {0.45f, 0.40f, 0.38f};
    glm::vec3 snowTint {0.95f, 0.95f, 0.98f};

    TerrainMaterial(const Texture2D* g, const Texture2D* r, const Texture2D* s,
                    float sl, float hs)
        : grass(g), rock(r), snow(s),
          snowLevel(sl), heightScale(hs) {}

    bool scatter(const Ray& /*in*/, const HitRecord& rec,
                 glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 dir = rec.normal + randomUnitVector();
        if (nearZero(dir)) dir = rec.normal;
        scattered = Ray(rec.point, glm::normalize(dir));

        // Slope = angle from world-up.
        float slope = 1.0f - std::fabs(rec.normal.y); // 0 = flat, 1 = cliff

        auto sampleOr = [](const Texture2D* t, float u, float v,
                           const glm::vec3& fallback) {
            return (t && t->loaded()) ? t->sample(u, v) : fallback;
        };

        // Repeat UVs a few times so a 1-unit cell isn't stretched over the whole map.
        float u = rec.uv.x * 16.0f;
        float v = rec.uv.y * 16.0f;

        glm::vec3 grassC = sampleOr(grass, u, v, grassTint);
        glm::vec3 rockC  = sampleOr(rock,  u, v, rockTint);
        glm::vec3 snowC  = sampleOr(snow,  u, v, snowTint);

        // Start from grass; blend to rock with slope.
        float rockMix = glm::smoothstep(0.25f, 0.55f, slope);
        glm::vec3 base = glm::mix(grassC, rockC, rockMix);

        // Above snowLevel, blend to snow — but steep slopes shed snow.
        float snowMix = glm::smoothstep(snowLevel - 1.0f, snowLevel + 1.5f, rec.point.y);
        snowMix *= (1.0f - glm::smoothstep(0.55f, 0.85f, slope));
        base = glm::mix(base, snowC, snowMix);

        attenuation = base;
        return true;
    }
};

// ----------------------------------------------------------------------------
// Public factories so other TUs don't need to know the concrete classes.
// ----------------------------------------------------------------------------
namespace Materials {
    Material* makeLambertian(const glm::vec3& albedo) {
        return new LambertianMaterial(albedo);
    }
    Material* makeReflective(const glm::vec3& albedo, float roughness) {
        return new ReflectiveMaterial(albedo, roughness);
    }
    Material* makeTerrain(const Texture2D* grass,
                          const Texture2D* rock,
                          const Texture2D* snow,
                          float snowLevelWorldY,
                          float heightScale) {
        return new TerrainMaterial(grass, rock, snow, snowLevelWorldY, heightScale);
    }
}
