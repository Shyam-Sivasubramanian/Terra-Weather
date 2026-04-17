#include "BVH.h"
#include <algorithm>
#include <cstring>
#include <random>

/**
 * @brief BVH Node - either internal (has children) or leaf (has primitives)
 */

/**
 * @brief BVH Implementation with Surface Area Heuristic (SAH) construction
 */
class BVH::Impl {
public:
    std::vector<BVHNode> nodes;
    std::vector<TrianglePrimitive> primitives;
    size_t nodeCount = 0;

    Impl() { nodes.reserve(100000); }

    /**
     * @brief Build BVH from triangles using SAH
     */
    void build(const std::vector<TrianglePrimitive>& tris) {
        primitives = tris;
        nodes.clear();
        nodes.emplace_back();  // Root node

        std::vector<uint32_t> indices(primitives.size());
        for (uint32_t i = 0; i < primitives.size(); i++) {
            indices[i] = i;
        }

        buildRecursive(0, indices.data(), indices.size(), 0);
    }

private:
    void buildRecursive(uint32_t nodeIndex, uint32_t* indices, size_t count, int depth) {
        if (count == 0) return;

        nodes[nodeIndex].box = computeBoundingBox(indices, count);

        // Leaf node optimization
        if (count <= 4 || depth > 64) {
            nodes[nodeIndex].isLeaf = true;
            nodes[nodeIndex].left = indices[0];  // First primitive index
            nodes[nodeIndex].right = static_cast<uint32_t>(count);  // Primitive count
            return;
        }

        // SAH-based split
        float minCost = count * nodes[nodeIndex].box.surfaceArea();
        int bestAxis = -1;
        int bestPos = -1;
        float bestSplit = 0.0f;

        AABB centroidBox;
        for (size_t i = 0; i < count; i++) {
            AABB primBox = primitives[indices[i]].boundingBox();
            centroidBox.expand((primBox.min + primBox.max) * 0.5f);
        }

        // Try splits along each axis
        for (int axis = 0; axis < 3; axis++) {
            float minC = centroidBox.min[axis];
            float maxC = centroidBox.max[axis];
            if (maxC - minC < 0.0001f) continue;

            // Bin the primitives
            constexpr int NUM_BINS = 16;
            struct Bin {
                AABB box;
                uint32_t count = 0;
            };
            Bin bins[NUM_BINS];

            float invRange = NUM_BINS / (maxC - minC);

            for (size_t i = 0; i < count; i++) {
                AABB primBox = primitives[indices[i]].boundingBox();
                float c = (primBox.min[axis] + primBox.max[axis]) * 0.5f;
                int bin = std::clamp(int((c - minC) * invRange), 0, NUM_BINS - 1);
                bins[bin].box.expand(primBox.min);
                bins[bin].box.expand(primBox.max);
                bins[bin].count++;
            }

            // Sweep to find best split
            AABB leftBox, rightBox;
            uint32_t leftCount = 0, rightCount = 0;

            for (int i = 0; i < NUM_BINS - 1; i++) {
                leftBox.expand(bins[i].box);
                leftCount += bins[i].count;

                rightBox.min = glm::vec3(std::numeric_limits<float>::infinity());
                rightBox.max = glm::vec3(-std::numeric_limits<float>::infinity());
                rightCount = 0;

                for (int j = i + 1; j < NUM_BINS; j++) {
                    rightBox.expand(bins[j].box);
                    rightCount += bins[j].count;
                }

                float cost = leftCount * leftBox.surfaceArea() +
                            rightCount * rightBox.surfaceArea();

                if (cost < minCost) {
                    minCost = cost;
                    bestAxis = axis;
                    bestPos = i;
                    bestSplit = minC + (bestPos + 1) / invRange;
                }
            }
        }

        // Create internal node
        nodes[nodeIndex].isLeaf = false;

        // Partition primitives
        std::vector<uint32_t> leftIndices;
        std::vector<uint32_t> rightIndices;

        if (bestAxis >= 0) {
            for (size_t i = 0; i < count; i++) {
                AABB primBox = primitives[indices[i]].boundingBox();
                float c = (primBox.min[bestAxis] + primBox.max[bestAxis]) * 0.5f;
                if (c < bestSplit) {
                    leftIndices.push_back(indices[i]);
                } else {
                    rightIndices.push_back(indices[i]);
                }
            }
        }

        // Fallback: split in half
        if (leftIndices.empty() || rightIndices.empty()) {
            size_t mid = count / 2;
            std::nth_element(indices, indices + mid, indices + count,
                [this, &bestAxis](uint32_t a, uint32_t b) {
                    AABB boxA = primitives[a].boundingBox();
                    AABB boxB = primitives[b].boundingBox();
                    return (boxA.min[bestAxis] + boxA.max[bestAxis]) <
                           (boxB.min[bestAxis] + boxB.max[bestAxis]);
                });
            mid = count / 2;
            leftIndices.assign(indices, indices + mid);
            rightIndices.assign(indices + mid, indices + count);
        }

        // Create child nodes
        nodes[nodeIndex].left = static_cast<uint32_t>(nodes.size());
        nodes.emplace_back();
        buildRecursive(nodes[nodeIndex].left, leftIndices.data(), leftIndices.size(), depth + 1);

        nodes[nodeIndex].right = static_cast<uint32_t>(nodes.size());
        nodes.emplace_back();
        buildRecursive(nodes[nodeIndex].right, rightIndices.data(), rightIndices.size(), depth + 1);
    }

    AABB computeBoundingBox(uint32_t* indices, size_t count) {
        AABB box;
        for (size_t i = 0; i < count; i++) {
            box.expand(primitives[indices[i]].boundingBox());
        }
        return box;
    }

public:
    /**
     * @brief Trace ray through BVH
     */
    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
        return hitRecursive(0, ray, tmin, tmax, rec);
    }

private:
    bool hitRecursive(uint32_t nodeIndex, const Ray& ray,
                     float tmin, float tmax, HitRecord& rec) const {
        const BVHNode& node = nodes[nodeIndex];

        if (!node.box.hit(ray, tmin, tmax)) return false;

        bool hitAnything = false;
        float closestT = tmax;

        if (node.isLeaf) {
            // Test all primitives in leaf
            for (uint32_t i = 0; i < node.right; i++) {
                HitRecord tempRec;
                if (primitives[node.left + i].hit(ray, tmin, closestT, tempRec)) {
                    if (tempRec.t < closestT) {
                        closestT = tempRec.t;
                        rec = tempRec;
                        hitAnything = true;
                    }
                }
            }
        } else {
            // Recursively test children
            HitRecord leftRec, rightRec;
            bool hitLeft = hitRecursive(node.left, ray, tmin, closestT, leftRec);
            bool hitRight = hitRecursive(node.right, ray, tmin, closestT, rightRec);

            if (hitLeft && leftRec.t < closestT) {
                closestT = leftRec.t;
                rec = leftRec;
                hitAnything = true;
            }
            if (hitRight && rightRec.t < closestT) {
                closestT = rightRec.t;
                rec = rightRec;
                hitAnything = true;
            }
        }

        return hitAnything;
    }
};

/**
 * @brief Triangle primitive for BVH
 */
AABB TrianglePrimitive::boundingBox() const {
    AABB box;
    box.expand(v0);
    box.expand(v1);
    box.expand(v2);
    return box;
}

bool TrianglePrimitive::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(ray.direction, edge2);
    float a = glm::dot(edge1, h);

    if (fabs(a) < 0.0001f) return false;

    float f = 1.0f / a;
    glm::vec3 s = ray.origin - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray.direction, q);

    if (v < 0.0f || u + v > 1.0f) return false;

    float t = f * glm::dot(edge2, q);

    if (t > tmin && t < tmax) {
        rec.t = t;
        rec.point = ray.at(t);
        rec.normal = glm::normalize(glm::cross(edge1, edge2));
        if (glm::dot(rec.normal, ray.direction) > 0) {
            rec.normal = -rec.normal;
        }
        rec.material = material;
        return true;
    }

    return false;
}

/**
 * @brief BVH Implementation
 */
BVH::BVH() : impl(std::make_unique<Impl>()) {}
BVH::~BVH() = default;

void BVH::build(const std::vector<TrianglePrimitive>& triangles) {
    impl->build(triangles);
}

bool BVH::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    return impl->hit(ray, tmin, tmax, rec);
}

size_t BVH::nodeCount() const {
    return impl->nodes.size();
}
