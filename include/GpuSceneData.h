#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "AABB.h"

// ============================================================================
// GPU-friendly flat data. All struct fields are aligned to std430 layout
// (vec4 alignment for vec3s) so the compute shader's SSBO can read them
// without surprises. Sizes here MUST match the shader's layout.
// ============================================================================

// 64 bytes. vec3 fields padded to vec4.
struct alignas(16) GpuBVHNode {
    glm::vec4 bmin;        // xyz = AABB min, w = leaf flag (1.0 = leaf)
    glm::vec4 bmax;        // xyz = AABB max, w unused
    // For internal nodes: left/right are child indices into the nodes array.
    // For leaf nodes:     firstTri/triCount index into the triangles array.
    int left;
    int right;
    int firstTri;
    int triCount;
};
static_assert(sizeof(GpuBVHNode) == 48, "Bad GpuBVHNode size");
// std430 pads to multiple of largest member (vec4 = 16), so actual GPU stride
// will be 48. We'll declare it as 48 on the GPU side too.

// 128 bytes per triangle (vec4-padded). Generous but simple.
struct alignas(16) GpuTri {
    glm::vec4 v0;     // xyz = position, w = matId
    glm::vec4 v1;
    glm::vec4 v2;
    glm::vec4 n0;     // xyz = normal, w = unused
    glm::vec4 n1;
    glm::vec4 n2;
    glm::vec4 uv;     // xy = uv0, zw = uv1
    glm::vec4 uv2_pad; // xy = uv2, zw = unused
};
static_assert(sizeof(GpuTri) == 128, "Bad GpuTri size");

struct GpuSceneData {
    std::vector<GpuBVHNode> nodes;
    std::vector<GpuTri>     tris;
    AABB                    worldBounds;
};
