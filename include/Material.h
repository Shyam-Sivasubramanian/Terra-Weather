#include "HitRecord.h"
#include "HitRecord.h"
#pragma once

#include "Ray.h"
#include "HitRecord.h"
#include "Texture.h"
#include "glm/glm.hpp"
#include <memory>
#include <random>
#include <cmath>

/**
 * @brief Abstract base class for all materials
 *
 * Materials define how rays interact with surfaces through the scatter() method.
 * Each material type implements its own scattering behavior (diffuse, specular, etc.)
 */
class Material {
public:
    virtual ~Material() = default;

    /**
     * @brief Compute scattered rays when a ray hits this material
     * @param rayIn The incoming ray that hit the surface
     * @param rec Hit record containing intersection details
     * @param attenuation Color absorption/modification (out parameter)
     * @param scattered The scattered/reflected ray (out parameter)
     * @return true if a ray was scattered, false if absorption occurred
     */
    virtual bool scatter(const Ray& rayIn, const HitRecord& rec,
                        glm::vec3& attenuation, Ray& scattered) const = 0;

    /**
     * @brief Get emission color for emissive materials
     */
    virtual glm::vec3 emitted(float u, float v, const glm::vec3& p) const {
        return glm::vec3(0.0f);
    }

    /**
     * @brief Get the scattering PDF for importance sampling
     */
    virtual float scatteringPDF(const Ray& rayIn, const HitRecord& rec,
                               const Ray& scattered) const {
        return 0.0f;
    }

    /**
     * @brief Check if this is an emissive material
     */
    virtual bool isEmissive() const { return false; }

protected:
    /**
     * @brief Generate a random direction in a hemisphere (cosine-weighted)
     */
    static glm::vec3 randomCosineDirection() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        float u1 = dis(gen);
        float u2 = dis(gen);
        float r = sqrt(u1);
        float theta = 2.0f * 3.14159265359f * u2;

        float x = r * cos(theta);
        float y = r * sin(theta);
        float z = sqrt(1.0f - u1);

        return glm::normalize(glm::vec3(x, y, z));
    }

    /**
     * @brief Generate a random direction in unit sphere
     */
    static glm::vec3 randomInUnitSphere() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

        while (true) {
            glm::vec3 p(dis(gen), dis(gen), dis(gen));
            if (glm::dot(p, p) < 1.0f) return p;
        }
    }

    /**
     * @brief Generate a random unit vector
     */
    static glm::vec3 randomUnitVector() {
        return glm::normalize(randomInUnitSphere());
    }

    /**
     * @brief Reflect a vector about a normal
     */
    static glm::vec3 reflect(const glm::vec3& v, const glm::vec3& n) {
        return v - 2.0f * glm::dot(v, n) * n;
    }

    /**
     * @brief Refract a vector using Snell's law
     */
    static bool refract(const glm::vec3& v, const glm::vec3& n, float ni_over_nt,
                       glm::vec3& refracted) {
        glm::vec3 uv = glm::normalize(v);
        float dt = glm::dot(uv, n);
        float discriminant = 1.0f - ni_over_nt * ni_over_nt * (1.0f - dt * dt);

        if (discriminant > 0) {
            refracted = ni_over_nt * (uv - n * dt) - n * sqrtf(discriminant);
            return true;
        }
        return false;
    }

    /**
     * @brief Schlick's approximation for Fresnel reflectance
     */
    static float schlick(float cosine, float refIdx) {
        float r0 = (1.0f - refIdx) / (1.0f + refIdx);
        r0 = r0 * r0;
        return r0 + (1.0f - r0) * pow(1.0f - cosine, 5.0f);
    }
};

/**
 * @brief Lambertian (diffuse) material
 */
class Lambertian : public Material {
public:
    glm::vec3 albedo;           // Base color
    std::shared_ptr<Texture> texture;  // Optional texture

    Lambertian(const glm::vec3& a) : albedo(a) {}
    Lambertian(std::shared_ptr<Texture> tex) : texture(tex) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 scatterDir = rec.normal + randomUnitVector();

        // Handle near-zero direction
        if (glm::length(scatterDir) < 0.001f) {
            scatterDir = rec.normal;
        }

        scattered = Ray(rec.point, glm::normalize(scatterDir));
        attenuation = getColor(rec.uv.xv.x, rec.uv.xv.y, rec.point);

        return true;
    }

    float scatteringPDF(const Ray& rayIn, const HitRecord& rec,
                        const Ray& scattered) const override {
        float cosine = glm::dot(rec.normal, glm::normalize(scattered.direction));
        if (cosine < 0) cosine = 0;
        return cosine / 3.14159265359f;
    }

    glm::vec3 getColor(float u, float v, const glm::vec3& p) const {
        if (texture) return texture->value(u, v, p);
        return albedo;
    }
};

/**
 * @brief Metal (reflective) material
 */
class Metal : public Material {
public:
    glm::vec3 albedo;      // Base color
    float fuzz;            // Fuzziness (0 = perfect mirror, 1 = fully diffuse reflection)

    Metal(const glm::vec3& a, float f = 0.0f) : albedo(a), fuzz(f) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 reflected = reflect(rayIn.direction, rec.normal);
        scattered = Ray(rec.point, glm::normalize(reflected + fuzz * randomInUnitSphere()));
        attenuation = albedo;

        // Only scatter if ray reflects in positive normal direction
        return glm::dot(scattered.direction, rec.normal) > 0;
    }
};

/**
 * @brief Dielectric (glass, water, etc.) material
 */
class Dielectric : public Material {
public:
    float refIdx;  // Refractive index

    Dielectric(float ri) : refIdx(ri) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 outwardNormal;
        float ni_over_nt;
        attenuation = glm::vec3(1.0f);

        glm::vec3 reflected = reflect(rayIn.direction, rec.normal);

        float reflectProb;
        float cosine;

        if (glm::dot(rayIn.direction, rec.normal) > 0) {
            outwardNormal = -rec.normal;
            ni_over_nt = refIdx;
            cosine = refIdx * glm::dot(rayIn.direction, rec.normal);
        } else {
            outwardNormal = rec.normal;
            ni_over_nt = 1.0f / refIdx;
            cosine = -glm::dot(rayIn.direction, rec.normal);
        }

        glm::vec3 refracted;
        if (refract(rayIn.direction, outwardNormal, ni_over_nt, refracted)) {
            reflectProb = schlick(cosine, refIdx);
        } else {
            reflectProb = 1.0f;
        }

        // Use reflection with probability reflectProb
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        if (dis(gen) < reflectProb) {
            scattered = Ray(rec.point, reflected);
        } else {
            scattered = Ray(rec.point, refracted);
        }

        return true;
    }
};

/**
 * @brief Isotropic (volume) scattering material
 */
class Isotropic : public Material {
public:
    std::shared_ptr<Texture> texture;

    Isotropic(std::shared_ptr<Texture> tex) : texture(tex) {}
    Isotropic(const glm::vec3& c) : texture(std::make_shared<SolidColor>(c)) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        scattered = Ray(rec.point, randomUnitVector());
        attenuation = texture->value(rec.uv.xv.x, rec.uv.xv.y, rec.point);
        return true;
    }
};

/**
 * @brief Diffuse light (emissive) material
 */
class DiffuseLight : public Material {
public:
    std::shared_ptr<Texture> texture;
    float intensity;

    DiffuseLight(std::shared_ptr<Texture> tex, float i = 1.0f)
        : texture(tex), intensity(i) {}
    DiffuseLight(const glm::vec3& c, float i = 1.0f)
        : texture(std::make_shared<SolidColor>(c)), intensity(i) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        return false;  // Light doesn't scatter
    }

    glm::vec3 emitted(float u, float v, const glm::vec3& p) const override {
        return intensity * texture->value(u, v, p);
    }

    bool isEmissive() const override { return true; }
};
