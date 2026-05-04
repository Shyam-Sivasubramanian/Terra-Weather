#include "BVH.h"

#include <algorithm>
#include <cstdlib>

namespace {
// Comparator factory for splitting by centroid along an axis.
bool boxCompare(const std::shared_ptr<Hittable>& a,
                const std::shared_ptr<Hittable>& b, int axis) {
    AABB ba, bb;
    if (!a->boundingBox(ba) || !b->boundingBox(bb)) return false;
    return ba.centroid()[axis] < bb.centroid()[axis];
}
} // namespace

BVHNode::BVHNode(std::vector<std::shared_ptr<Hittable>>& objects,
                 std::size_t start, std::size_t end) {
    std::size_t n = end - start;
    if (n == 0) {
        // Empty: construct a degenerate box; hit() will early-out.
        box_ = AABB(glm::vec3(0), glm::vec3(0));
        return;
    }
    if (n == 1) {
        left_ = right_ = objects[start];
    } else if (n == 2) {
        left_  = objects[start];
        right_ = objects[start + 1];
    } else {
        // Choose the longest axis of the parent centroid bound for splitting.
        AABB cbox;
        for (std::size_t i = start; i < end; ++i) {
            AABB b;
            if (objects[i]->boundingBox(b)) {
                cbox.expand(b.centroid());
            }
        }
        int axis = cbox.longestAxis();
        auto cmp = [axis](const std::shared_ptr<Hittable>& a,
                          const std::shared_ptr<Hittable>& b) {
            return boxCompare(a, b, axis);
        };
        std::sort(objects.begin() + start, objects.begin() + end, cmp);
        std::size_t mid = start + n / 2;
        left_  = std::make_shared<BVHNode>(objects, start, mid);
        right_ = std::make_shared<BVHNode>(objects, mid,   end);
    }

    AABB bl, br;
    bool okL = left_  ? left_->boundingBox(bl)  : false;
    bool okR = right_ ? right_->boundingBox(br) : false;
    if (okL && okR) box_ = surroundingBox(bl, br);
    else if (okL)   box_ = bl;
    else if (okR)   box_ = br;
}

bool BVHNode::hit(const Ray& r, float tmin, float tmax, HitRecord& rec) const {
    if (!box_.hit(r, tmin, tmax)) return false;

    bool hitLeft  = left_  && left_->hit(r, tmin, tmax, rec);
    // If we hit on the left, tighten tmax for the right child.
    bool hitRight = right_ && right_->hit(r, tmin, hitLeft ? rec.t : tmax, rec);
    return hitLeft || hitRight;
}

bool BVHNode::boundingBox(AABB& out) const {
    out = box_;
    return true;
}
