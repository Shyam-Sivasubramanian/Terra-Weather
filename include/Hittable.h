#pragma once

#include "Ray.h"
#include "HitRecord.h"
#include "AABB.h"

class Hittable {
public:
    virtual ~Hittable() = default;
    virtual bool hit(const Ray& r, float tmin, float tmax, HitRecord& rec) const = 0;
    virtual bool boundingBox(AABB& out) const = 0;
};
