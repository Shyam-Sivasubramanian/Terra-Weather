#include "Materials.h"
#include <random>
#include <cmath>

/**
 * @brief Lambertian (diffuse) material implementation
 */
class LambertianMaterial : public Material {
public:
    glm::vec3 albedo;
    std::shared_ptr<Texture> texture;
    bool useTexture;

    LambertianMaterial(const glm::vec3& a) : albedo(a), useTexture(false) {}
    LambertianMaterial(std::shared_ptr<Texture> tex) : texture(tex), useTexture(true) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 scatterDir = rec.normal + randomUnitVector();

        // Handle near-zero direction (degenerate case)
        if (glm::length(scatterDir) < 0.001f) {
            scatterDir = rec.normal;
        }

        scattered = Ray(rec.point, glm::normalize(scatterDir));

        if (useTexture) {
            attenuation = texture->value(rec.uv.x, rec.uv.y, rec.point);
        } else {
            attenuation = albedo;
        }

        return true;
    }

    float scatteringPDF(const Ray& rayIn, const HitRecord& rec,
                       const Ray& scattered) const override {
        float cosine = glm::dot(rec.normal, glm::normalize(scattered.direction));
        return glm::max(cosine, 0.0f) / 3.14159265359f;
    }
};

/**
 * @brief Metal (reflective) material implementation
 */
class MetalMaterial : public Material {
public:
    glm::vec3 albedo;
    float fuzz;  // Fuzz factor (0 = perfect mirror)

    MetalMaterial(const glm::vec3& a, float f = 0.0f) : albedo(a), fuzz(f) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 reflected = reflect(rayIn.direction, rec.normal);

        // Add fuzz reflection
        if (fuzz > 0.0f) {
            reflected += fuzz * randomInUnitSphere();
        }

        scattered = Ray(rec.point, glm::normalize(reflected));
        attenuation = albedo;

        // Only scatter if ray reflects in positive normal direction
        return glm::dot(scattered.direction, rec.normal) > 0;
    }
};

/**
 * @brief Dielectric (glass/water) material implementation
 */
class DielectricMaterial : public Material {
public:
    float refIdx;  // Refractive index

    DielectricMaterial(float ri) : refIdx(ri) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        glm::vec3 outwardNormal;
        float ni_over_nt;
        attenuation = glm::vec3(1.0f);

        // Determine if entering or exiting
        if (rec.frontFace) {
            outwardNormal = -rec.normal;
            ni_over_nt = 1.0f / refIdx;
        } else {
            outwardNormal = rec.normal;
            ni_over_nt = refIdx;
        }

        glm::vec3 refracted;
        float reflectProb;

        if (refract(rayIn.direction, outwardNormal, ni_over_nt, refracted)) {
            // Use Schlick's approximation
            float cosine = glm::dot(-rayIn.direction, rec.normal);
            reflectProb = schlick(cosine, refIdx);
        } else {
            // Total internal reflection
            reflectProb = 1.0f;
        }

        // Stochastic reflection/refraction
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        if (dis(gen) < reflectProb) {
            glm::vec3 reflected = reflect(rayIn.direction, rec.normal);
            scattered = Ray(rec.point, reflected);
        } else {
            scattered = Ray(rec.point, refracted);
        }

        return true;
    }
};

/**
 * @brief Emissive material for lights
 */
class EmissiveMaterial : public Material {
public:
    glm::vec3 color;
    float intensity;

    EmissiveMaterial(const glm::vec3& c, float i = 1.0f) : color(c), intensity(i) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        return false;  // Light sources don't scatter
    }

    glm::vec3 emitted(float u, float v, const glm::vec3& p) const override {
        return intensity * color;
    }

    bool isEmissive() const override { return true; }
};

/**
 * @brief Isotropic scattering material (for volumes)
 */
class IsotropicMaterial : public Material {
public:
    std::shared_ptr<Texture> texture;

    IsotropicMaterial(std::shared_ptr<Texture> tex) : texture(tex) {}
    IsotropicMaterial(const glm::vec3& c)
        : texture(std::make_shared<SolidColor>(c)) {}

    bool scatter(const Ray& rayIn, const HitRecord& rec,
                glm::vec3& attenuation, Ray& scattered) const override {
        scattered = Ray(rec.point, randomUnitVector());
        attenuation = texture->value(rec.uv.x, rec.uv.y, rec.point);
        return true;
    }
};

/**
 * @brief Material factory functions
 */
namespace MaterialFactory {
    std::shared_ptr<Material> createLambertian(const glm::vec3& albedo) {
        return std::make_shared<LambertianMaterial>(albedo);
    }

    std::shared_ptr<Material> createLambertian(std::shared_ptr<Texture> texture) {
        return std::make_shared<LambertianMaterial>(texture);
    }

    std::shared_ptr<Material> createMetal(const glm::vec3& albedo, float fuzz) {
        return std::make_shared<MetalMaterial>(albedo, fuzz);
    }

    std::shared_ptr<Material> createDielectric(float refIdx) {
        return std::make_shared<DielectricMaterial>(refIdx);
    }

    std::shared_ptr<Material> createEmissive(const glm::vec3& color, float intensity) {
        return std::make_shared<EmissiveMaterial>(color, intensity);
    }

    std::shared_ptr<Material> createIsotropic(const glm::vec3& color) {
        return std::make_shared<IsotropicMaterial>(color);
    }

    std::shared_ptr<Material> createIsotropic(std::shared_ptr<Texture> texture) {
        return std::make_shared<IsotropicMaterial>(texture);
    }

    /**
     * @brief Create terrain material based on height and climate
     */
    std::shared_ptr<Material> createTerrainMaterial(float height, float humidity,
                                                      float temperature, float slope) {
        // Water
        if (height < 0.35f) {
            return createDielectric(1.33f);  // Water IOR
        }

        // Snow
        if (height > 0.85f) {
            return createLambertian(glm::vec3(0.95f, 0.97f, 1.0f));
        }

        // Rock (steep slopes)
        if (slope > 0.7f) {
            return createLambertian(glm::vec3(0.4f, 0.35f, 0.3f));
        }

        // Grass based on humidity/temperature
        glm::vec3 grassColor = glm::mix(
            glm::vec3(0.7f, 0.6f, 0.3f),  // Dry
            glm::vec3(0.2f, 0.5f, 0.15f), // Wet
            humidity
        );
        grassColor = glm::mix(grassColor, glm::vec3(0.4f, 0.45f, 0.35f), 1.0f - temperature);

        return createLambertian(grassColor);
    }
}

/**
 * @brief Texture implementation
 */

// Solid color texture
glm::vec3 SolidColor::value(float u, float v, const glm::vec3& p) const {
    return color;
}

// Noise texture
float NoiseTexture::noiseFunc(const glm::vec3& p) const {
    // Simple value noise implementation
    // In production, use Perlin or Simplex noise
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    float ix = floor(p.x);
    float iy = floor(p.y);
    float iz = floor(p.z);

    float fx = p.x - ix;
    float fy = p.y - iy;
    float fz = p.z - iz;

    // Simple hash
    auto hash = [](float x, float y, float z) -> float {
        std::mt19937 gen(static_cast<unsigned>(x * 1000 + y * 100 + z));
        return dis(gen);
    };

    float v000 = hash(ix, iy, iz);
    float v100 = hash(ix + 1, iy, iz);
    float v010 = hash(ix, iy + 1, iz);
    float v110 = hash(ix + 1, iy + 1, iz);
    float v001 = hash(ix, iy, iz + 1);
    float v101 = hash(ix + 1, iy, iz + 1);
    float v011 = hash(ix, iy + 1, iz + 1);
    float v111 = hash(ix + 1, iy + 1, iz + 1);

    // Trilinear interpolation
    float x1 = v000 * (1 - fx) + v100 * fx;
    float x2 = v010 * (1 - fx) + v110 * fx;
    float x3 = v001 * (1 - fx) + v101 * fx;
    float x4 = v011 * (1 - fx) + v111 * fx;

    float y1 = x1 * (1 - fy) + x2 * fy;
    float y2 = x3 * (1 - fy) + x4 * fy;

    return y1 * (1 - fz) + y2 * fz;
}

float NoiseTexture::turbulenceFunc(const glm::vec3& p, int oct) const {
    float sum = 0.0f;
    float freq = 1.0f;
    float amp = 1.0f;

    for (int i = 0; i < oct; i++) {
        sum += amp * noiseFunc(p * freq);
        freq *= 2.0f;
        amp *= 0.5f;
    }

    return sum;
}

// Image texture
bool ImageTexture::load(const std::string& filename) {
    // Would use stb_image for actual implementation
    // For now, return false
    return false;
}
