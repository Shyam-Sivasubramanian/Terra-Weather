#pragma once

#include "HitRecord.h"
#include "Material.h"
#include "Ray.h"
#include "glm/glm.hpp"
#include <memory>
#include <vector>

/**
 * @brief Bounding Volume Hierarchy for ray tracing acceleration
 *
 * Uses Surface Area Heuristic (SAH) for optimal split selection.
 * Provides O(log n) average case ray intersection performance.
 */
class BVH {
public:
    BVH();
    ~BVH();

    /**
     * @brief Build BVH from triangle array
     */
    void build(const std::vector<TrianglePrimitive>& triangles);

    /**
     * @brief Test ray intersection with BVH
     */
    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const;

    /**
     * @brief Get number of nodes in BVH
     */
    size_t nodeCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Triangle primitive for BVH construction
 */
struct TrianglePrimitive {
    glm::vec3 v0, v1, v2;
    std::shared_ptr<Material> material;
    int gridX, gridZ;  // Optional grid position for texture lookup

    AABB boundingBox() const;
    bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const;
};

/**
 * @brief Axis-aligned bounding box tree node
 */
struct BVHNode {
    AABB box;
    uint32_t left;     // Left child index
    uint32_t right;     // Right child index or primitive count
    uint32_t firstPrim; // First primitive (for leaf)
    uint32_t primCount; // Number of primitives (for leaf)
    bool isLeaf;

    BVHNode() : left(0), right(0), firstPrim(0), primCount(0), isLeaf(true) {}
};

/**
 * @brief Optimized BVH builder with SAH
 */
class BVHBuilder {
public:
    static BVHBuilder& instance() {
        static BVHBuilder builder;
        return builder;
    }

    void build(std::vector<BVHNode>& nodes, const std::vector<TrianglePrimitive>& primitives);

private:
    void buildRecursive(std::vector<BVHNode>& nodes,
                        const std::vector<TrianglePrimitive>& primitives,
                        uint32_t* indices, size_t count,
                        int depth);
};
