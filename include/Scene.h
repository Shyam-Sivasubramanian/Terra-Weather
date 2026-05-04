#pragma once

#include <memory>
#include <vector>
#include "Hittable.h"
#include "Texture2D.h"

struct WorldData;
class Material;

// Scene owns:
//  - Material*s (heap-allocated, deleted in dtor)
//  - Texture2Ds
//  - TerrainChunk hittables (via shared_ptr)
//  - The BVH root, also a Hittable.
class Scene {
public:
    Scene() = default;
    ~Scene();

    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;

    // Rebuild the scene from current WorldData. Safe to call on seed change.
    // Texture paths may be empty; materials fall back to solid colors.
    void build(const WorldData& world,
               const char* grassPath = nullptr,
               const char* rockPath  = nullptr,
               const char* snowPath  = nullptr);

    // Query: BVH-accelerated ray-scene intersection.
    bool hit(const Ray& r, float tmin, float tmax, HitRecord& rec) const;

    // World-space bounds (for atmosphere / fog attenuation).
    AABB worldBounds() const { return bounds_; }

private:
    std::vector<std::shared_ptr<Hittable>> objects_;
    std::shared_ptr<Hittable>              bvh_;
    std::vector<Material*>                 materials_;
    Texture2D grassTex_, rockTex_, snowTex_;
    AABB bounds_;

    void clear();
};
