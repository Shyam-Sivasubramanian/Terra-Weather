#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include "Ray.h"

struct AABB {
    glm::vec3 min{ 1e30f,  1e30f,  1e30f};
    glm::vec3 max{-1e30f, -1e30f, -1e30f};

    AABB() = default;
    AABB(const glm::vec3& a, const glm::vec3& b) : min(glm::min(a,b)), max(glm::max(a,b)) {}

    // Andrew Kensler's slab-test variant; robust against NaNs from 1/0 direction components.
    inline bool hit(const Ray& r, float tmin, float tmax) const {
        for (int a = 0; a < 3; ++a) {
            float invD = 1.0f / r.direction[a];
            float t0 = (min[a] - r.origin[a]) * invD;
            float t1 = (max[a] - r.origin[a]) * invD;
            if (invD < 0.0f) std::swap(t0, t1);
            tmin = t0 > tmin ? t0 : tmin;
            tmax = t1 < tmax ? t1 : tmax;
            if (tmax <= tmin) return false;
        }
        return true;
    }

    inline glm::vec3 centroid() const { return 0.5f * (min + max); }

    inline void expand(const glm::vec3& p) {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }

    inline void expand(const AABB& o) {
        min = glm::min(min, o.min);
        max = glm::max(max, o.max);
    }

    inline int longestAxis() const {
        glm::vec3 d = max - min;
        if (d.x > d.y && d.x > d.z) return 0;
        if (d.y > d.z) return 1;
        return 2;
    }
};

inline AABB surroundingBox(const AABB& a, const AABB& b) {
    AABB r = a;
    r.expand(b);
    return r;
}
