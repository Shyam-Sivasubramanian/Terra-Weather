#pragma once

#include <vector>
#include <memory>
#include "Hittable.h"

// Simple BVH over a list of Hittable*s.
// Owns the hittables via shared_ptr. Built once, then queried per ray.
class BVHNode : public Hittable {
public:
    BVHNode() = default;
    BVHNode(std::vector<std::shared_ptr<Hittable>>& objects,
            std::size_t start, std::size_t end);

    bool hit(const Ray& r, float tmin, float tmax, HitRecord& rec) const override;
    bool boundingBox(AABB& out) const override;

private:
    std::shared_ptr<Hittable> left_;
    std::shared_ptr<Hittable> right_;
    AABB box_;
};
