#pragma once

#include "Material.h"
#include "Texture.h"
#include "Ray.h"
#include "HitRecord.h"
#include "glm/glm.hpp"
#include <memory>
#include <string>

/**
 * @brief Material factory namespace
 *
 * Provides factory functions for creating common material types.
 */
namespace MaterialFactory {
    std::shared_ptr<Material> createLambertian(const glm::vec3& albedo);
    std::shared_ptr<Material> createLambertian(std::shared_ptr<Texture> texture);
    std::shared_ptr<Material> createMetal(const glm::vec3& albedo, float fuzz = 0.0f);
    std::shared_ptr<Material> createDielectric(float refIdx);
    std::shared_ptr<Material> createEmissive(const glm::vec3& color, float intensity = 1.0f);
    std::shared_ptr<Material> createIsotropic(const glm::vec3& color);
    std::shared_ptr<Material> createIsotropic(std::shared_ptr<Texture> texture);

    /**
     * @brief Create terrain material based on height and climate
     */
    std::shared_ptr<Material> createTerrainMaterial(float height, float humidity,
                                                      float temperature, float slope);
}

/**
 * @brief Terrain material that responds to world data
 */
class TerrainMaterial : public Material {
public:
    TerrainMaterial(const std::shared_ptr<Material>& water,
                   const std::shared_ptr<Material>& sand,
                   const std::shared_ptr<Material>& grass,
                   const std::shared_ptr<Material>& rock,
                   const std::shared_ptr<Material>& snow)
        : waterMat(water), sandMat(sand), grassMat(grass), rockMat(rock), snowMat(snow) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        // Delegate to appropriate material based on height
        float h = rec.point.y;

        if (h < 0.35f) return waterMat->scatter(rayIn, rec, attenuation, scattered);
        if (h < 0.37f) return sandMat->scatter(rayIn, rec, attenuation, scattered);
        if (h < 0.85f) return grassMat->scatter(rayIn, rec, attenuation, scattered);
        if (h < 0.9f) return rockMat->scatter(rayIn, rec, attenuation, scattered);
        return snowMat->scatter(rayIn, rec, attenuation, scattered);
    }

private:
    std::shared_ptr<Material> waterMat, sandMat, grassMat, rockMat, snowMat;
};

/**
 * @brief Water material with reflection and refraction
 */
class WaterMaterial : public Material {
public:
    WaterMaterial(const glm::vec3& deepColor, const glm::vec3& shallowColor,
                 float ior = 1.33f);

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override;

    glm::vec3 deepColor;
    glm::vec3 shallowColor;
    float ior;
    float waterLevel;

private:
    float schlick(float cosine, float r0) const;
};

/**
 * @brief Animated water material with waves
 */
class AnimatedWaterMaterial : public WaterMaterial {
public:
    AnimatedWaterMaterial(float waveAmplitude, float waveFrequency,
                         const glm::vec3& deepColor, const glm::vec3& shallowColor);

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override;

    float waveAmplitude;
    float waveFrequency;

private:
    float waveHeight(const glm::vec3& pos, float time) const;
};

/**
 * @brief Checkerboard material
 */
class CheckerMaterial : public Material {
public:
    CheckerMaterial(const glm::vec3& c1, const glm::vec3& c2, float scale = 1.0f)
        : color1(c1), color2(c2), scale(scale) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        float s = scale * rec.point.x + scale * rec.point.y + scale * rec.point.z;
        bool isEven = (int(floor(s)) % 2) == 0;

        scattered = Ray(rec.point, rec.normal + randomUnitVector());
        attenuation = isEven ? color1 : color2;
        return true;
    }

private:
    glm::vec3 color1, color2;
    float scale;
};

/**
 * @brief Normal map material (bumps normals)
 */
class NormalMapMaterial : public Material {
public:
    NormalMapMaterial(std::shared_ptr<Material> base, std::shared_ptr<Texture> normalMap)
        : baseMaterial(base), normalMap(normalMap) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        // Perturb normal using normal map
        glm::vec3 normalMapVal = normalMap->value(rec.uv.x, rec.uv.y, rec.point);
        glm::vec3 perturbedNormal = glm::normalize(rec.normal + (normalMapVal - 0.5f) * 2.0f);

        HitRecord perturbedRec = rec;
        perturbedRec.normal = perturbedNormal;

        return baseMaterial->scatter(rayIn, perturbedRec, attenuation, scattered);
    }

private:
    std::shared_ptr<Material> baseMaterial;
    std::shared_ptr<Texture> normalMap;
};
