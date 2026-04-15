#pragma once

#include "Ray.h"
#include "Material.h"
#include <memory>

/**
 * @brief Hit record structure for ray intersection results
 *
 * Contains all information about a ray-surface intersection point.
 * Passed by reference to intersection functions to fill in hit details.
 */
struct HitRecord {
    float t;                     // Intersection parameter along ray (ray.at(t) = hit point)
    glm::vec3 point;             // World-space hit point
    glm::vec3 normal;            // Surface normal at hit point (always outward from surface)
    glm::vec2 uv;               // Texture coordinates at hit point
    std::shared_ptr<Material> material;  // Material at hit point
    bool frontFace;             // True if ray hit front face (ray outside -> surface normal)

    // For volume scattering
    float density = 0.0f;        // Volume density at hit point
    bool isVolume = false;      // True if this is a volume hit

    HitRecord() : t(std::numeric_limits<float>::infinity()),
                  point(0.0f), normal(0.0f, 1.0f, 0.0f),
                  uv(0.0f, 0.0f), frontFace(true) {}

    /**
     * @brief Set hit record data
     */
    void set(float t_, const glm::vec3& point_, const glm::vec3& outwardNormal,
             const glm::vec2& uv_, std::shared_ptr<Material> mat) {
        t = t_;
        point = point_;
        uv = uv_;
        material = mat;
        frontFace = glm::dot(outwardNormal, point_ - point_) > 0;
        normal = frontFace ? -outwardNormal : outwardNormal;
    }

    /**
     * @brief Set normal ensuring it faces the incoming ray direction
     * @param r The ray that hit the surface
     * @param outwardNormal The geometric normal (pointing away from surface center)
     */
    void setFaceNormal(const Ray& r, const glm::vec3& outwardNormal) {
        frontFace = glm::dot(r.direction, outwardNormal) < 0;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }

    /**
     * @brief Check if this hit is valid
     */
    bool valid() const {
        return t > 0 && t < std::numeric_limits<float>::infinity();
    }
};

/**
 * @brief Axis-aligned bounding box for acceleration structures
 */
struct AABB {
    glm::vec3 min;  // Minimum corner
    glm::vec3 max;  // Maximum corner

    AABB() : min(std::numeric_limits<float>::infinity()),
             max(-std::numeric_limits<float>::infinity()) {}

    AABB(const glm::vec3& min_, const glm::vec3& max_) : min(min_), max(max_) {}

    /**
     * @brief Get center of bounding box
     */
    glm::vec3 center() const { return (min + max) * 0.5f; }

    /**
     * @brief Get extent (size) of bounding box
     */
    glm::vec3 extent() const { return max - min; }

    /**
     * @brief Check if ray intersects bounding box
     * @return true if intersection occurs within [tmin, tmax]
     */
    bool hit(const Ray& ray, float tmin, float tmax) const {
        for (int i = 0; i < 3; i++) {
            float invD = 1.0f / ray.direction[i];
            float t0 = (min[i] - ray.origin[i]) * invD;
            float t1 = (max[i] - ray.origin[i]) * invD;

            if (invD < 0.0f) std::swap(t0, t1);

            tmin = glm::max(tmin, t0);
            tmax = glm::min(tmax, t1);

            if (tmax <= tmin) return false;
        }
        return true;
    }

    /**
     * @brief Expand bounding box to include a point
     */
    void expand(const glm::vec3& p) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }

    /**
     * @brief Expand bounding box to include another bounding box
     */
    void expand(const AABB& box) {
        min = glm::min(min, box.min);
        max = glm::max(max, box.max);
    }

    /**
     * @brief Surface area heuristic for BVH construction
     */
    float surfaceArea() const {
        glm::vec3 e = max - min;
        return 2.0f * (e.x * e.y + e.y * e.z + e.z * e.x);
    }

    /**
     * @brief Check if a point is inside the bounding box
     */
    bool contains(const glm::vec3& p) const {
        return (p.x >= min.x && p.x <= max.x &&
                p.y >= min.y && p.y <= max.y &&
                p.z >= min.z && p.z <= max.z);
    }
};

/**
 * @brief Ray hit test interface for hittable objects
 */
class Hittable {
public:
    virtual ~Hittable() = default;

    /**
     * @brief Test ray intersection with this object
     * @param ray The ray to test
     * @param tmin Minimum valid t value
     * @param tmax Maximum valid t value
     * @param rec Hit record to fill if intersection occurs
     * @return true if intersection occurs
     */
    virtual bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const = 0;

    /**
     * @brief Get bounding box for this hittable
     */
    virtual AABB boundingBox(float time0, float time1) const = 0;
};

/**
 * @brief Translation of hittable object
 */
class Translate : public Hittable {
    std::shared_ptr<Hittable> ptr;
    glm::vec3 offset;

public:
    Translate(std::shared_ptr<Hittable> p, const glm::vec3& displacement)
        : ptr(p), offset(displacement) {}

    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const override {
        Ray moved(ray.origin - offset, ray.direction);
        if (!ptr->hit(moved, tmin, tmax, rec)) return false;
        rec.point += offset;
        return true;
    }

    AABB boundingBox(float time0, float time1) const override {
        AABB box = ptr->boundingBox(time0, time1);
        return AABB(box.min + offset, box.max + offset);
    }
};
