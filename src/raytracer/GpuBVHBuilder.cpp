#include "GpuBVHBuilder.h"
#include "WorldData.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <cstdio>

namespace GpuBVHBuilder {

namespace {

// Intermediate CPU representation during build. Each triangle carries its
// AABB and centroid for the split heuristic.
struct BuildTri {
    glm::vec3 v0, v1, v2;
    glm::vec3 n0, n1, n2;
    glm::vec2 uv0, uv1, uv2;
    int  matId;
    AABB bounds;
    glm::vec3 centroid;
};

// Grid -> world-space position, matching TerrainHittable's layout exactly so
// CPU and GPU renderers show the same geometry.
glm::vec3 gridToWorld(const WorldData& w, int gx, int gz) {
    float fx = static_cast<float>(gx) / std::max(1, w.width  - 1);
    float fz = static_cast<float>(gz) / std::max(1, w.height - 1);
    float x = (fx - 0.5f) * w.worldScale;
    float z = (fz - 0.5f) * w.worldScale;
    float hVal = w.heightMap.empty() ? 0.0f
               : w.heightMap[static_cast<std::size_t>(gz) * w.width + gx];
    return glm::vec3(x, hVal * w.heightScale, z);
}

std::vector<glm::vec3> vertexNormals(const WorldData& w) {
    std::vector<glm::vec3> N(static_cast<std::size_t>(w.width) * w.height,
                              glm::vec3(0.0f));
    for (int z = 0; z < w.height - 1; ++z) {
        for (int x = 0; x < w.width - 1; ++x) {
            glm::vec3 p00 = gridToWorld(w, x,     z    );
            glm::vec3 p10 = gridToWorld(w, x + 1, z    );
            glm::vec3 p01 = gridToWorld(w, x,     z + 1);
            glm::vec3 p11 = gridToWorld(w, x + 1, z + 1);
            glm::vec3 n1 = glm::normalize(glm::cross(p10 - p00, p01 - p00));
            glm::vec3 n2 = glm::normalize(glm::cross(p11 - p10, p01 - p10));
            auto idx = [&](int xi, int zi) {
                return static_cast<std::size_t>(zi) * w.width + xi;
            };
            N[idx(x,     z    )] += n1;
            N[idx(x + 1, z    )] += n1 + n2;
            N[idx(x,     z + 1)] += n1 + n2;
            N[idx(x + 1, z + 1)] += n2;
        }
    }
    for (auto& n : N) {
        float l2 = glm::dot(n, n);
        n = (l2 > 1e-12f) ? n / std::sqrt(l2) : glm::vec3(0, 1, 0);
    }
    return N;
}

std::vector<BuildTri> triangulate(const WorldData& world) {
    std::vector<BuildTri> out;
    if (world.width < 2 || world.height < 2) return out;
    out.reserve(static_cast<std::size_t>(world.width - 1) *
                static_cast<std::size_t>(world.height - 1) * 2);

    auto N = vertexNormals(world);
    auto ni = [&](int x, int z) {
        return N[static_cast<std::size_t>(z) * world.width + x];
    };
    const float seaY = world.seaLevel * world.heightScale;

    for (int z = 0; z < world.height - 1; ++z) {
        for (int x = 0; x < world.width - 1; ++x) {
            glm::vec3 p00 = gridToWorld(world, x,     z    );
            glm::vec3 p10 = gridToWorld(world, x + 1, z    );
            glm::vec3 p01 = gridToWorld(world, x,     z + 1);
            glm::vec3 p11 = gridToWorld(world, x + 1, z + 1);

            bool underwater = 0.25f * (p00.y + p10.y + p01.y + p11.y) < seaY;
            if (underwater) {
                p00.y = p10.y = p01.y = p11.y = seaY;
            }
            int mat = underwater ? MAT_WATER : MAT_TERRAIN;

            glm::vec3 wn(0, 1, 0);
            glm::vec3 n00 = underwater ? wn : ni(x,     z    );
            glm::vec3 n10 = underwater ? wn : ni(x + 1, z    );
            glm::vec3 n01 = underwater ? wn : ni(x,     z + 1);
            glm::vec3 n11 = underwater ? wn : ni(x + 1, z + 1);

            glm::vec2 uv00(static_cast<float>(x)     / (world.width  - 1),
                           static_cast<float>(z)     / (world.height - 1));
            glm::vec2 uv10(static_cast<float>(x + 1) / (world.width  - 1),
                           static_cast<float>(z)     / (world.height - 1));
            glm::vec2 uv01(static_cast<float>(x)     / (world.width  - 1),
                           static_cast<float>(z + 1) / (world.height - 1));
            glm::vec2 uv11(static_cast<float>(x + 1) / (world.width  - 1),
                           static_cast<float>(z + 1) / (world.height - 1));

            auto addTri = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
                              const glm::vec3& na, const glm::vec3& nb, const glm::vec3& nc,
                              const glm::vec2& ua, const glm::vec2& ub, const glm::vec2& uc) {
                BuildTri t;
                t.v0 = a; t.v1 = b; t.v2 = c;
                t.n0 = na; t.n1 = nb; t.n2 = nc;
                t.uv0 = ua; t.uv1 = ub; t.uv2 = uc;
                t.matId = mat;
                t.bounds = AABB();
                t.bounds.expand(a); t.bounds.expand(b); t.bounds.expand(c);
                // Guard against zero-volume bounds for degenerate triangles.
                const float eps = 1e-4f;
                for (int ax = 0; ax < 3; ++ax) {
                    if (t.bounds.max[ax] - t.bounds.min[ax] < eps) {
                        t.bounds.min[ax] -= eps;
                        t.bounds.max[ax] += eps;
                    }
                }
                t.centroid = (a + b + c) * (1.0f / 3.0f);
                out.push_back(t);
            };
            addTri(p00, p10, p11, n00, n10, n11, uv00, uv10, uv11);
            addTri(p00, p11, p01, n00, n11, n01, uv00, uv11, uv01);
        }
    }
    return out;
}

// Recursive median-split build. Outputs into a flat node array via emplace.
// Returns the index of the node it just wrote.
int buildRecursive(std::vector<BuildTri>& tris, int start, int end,
                   std::vector<GpuBVHNode>& nodes) {
    const int kLeafThreshold = 4; // <= N tris per leaf

    // Allocate this node's slot first so children see a consistent index.
    int nodeIdx = static_cast<int>(nodes.size());
    nodes.emplace_back();

    AABB bounds;
    AABB centroidBounds;
    for (int i = start; i < end; ++i) {
        bounds.expand(tris[i].bounds);
        centroidBounds.expand(tris[i].centroid);
    }

    int count = end - start;

    GpuBVHNode& n = nodes[nodeIdx];
    n.bmin = glm::vec4(bounds.min, 0.0f);
    n.bmax = glm::vec4(bounds.max, 0.0f);

    if (count <= kLeafThreshold) {
        // Leaf
        n.bmin.w = 1.0f;
        n.left = n.right = -1;
        n.firstTri = start;
        n.triCount = count;
        return nodeIdx;
    }

    // Internal — split along longest centroid axis at median.
    int axis = centroidBounds.longestAxis();
    std::nth_element(tris.begin() + start, tris.begin() + (start + count / 2),
                     tris.begin() + end,
                     [axis](const BuildTri& a, const BuildTri& b) {
                         return a.centroid[axis] < b.centroid[axis];
                     });
    int mid = start + count / 2;

    int l = buildRecursive(tris, start, mid, nodes);
    int r = buildRecursive(tris, mid,   end, nodes);

    // Re-fetch after recursion: the vector may have reallocated.
    nodes[nodeIdx].bmin.w = 0.0f;
    nodes[nodeIdx].left  = l;
    nodes[nodeIdx].right = r;
    nodes[nodeIdx].firstTri = -1;
    nodes[nodeIdx].triCount = 0;
    return nodeIdx;
}

} // namespace

GpuSceneData build(const WorldData& world) {
    GpuSceneData out;
    auto tris = triangulate(world);
    if (tris.empty()) return out;

    // Build the BVH. After build, tris has been partitioned in-place so leaf
    // ranges refer to contiguous triangle spans.
    out.nodes.reserve(tris.size() * 2);
    buildRecursive(tris, 0, static_cast<int>(tris.size()), out.nodes);

    // Pack triangles into GPU layout.
    out.tris.resize(tris.size());
    for (std::size_t i = 0; i < tris.size(); ++i) {
        const BuildTri& s = tris[i];
        GpuTri& d = out.tris[i];
        d.v0 = glm::vec4(s.v0, static_cast<float>(s.matId));
        d.v1 = glm::vec4(s.v1, 0.0f);
        d.v2 = glm::vec4(s.v2, 0.0f);
        d.n0 = glm::vec4(s.n0, 0.0f);
        d.n1 = glm::vec4(s.n1, 0.0f);
        d.n2 = glm::vec4(s.n2, 0.0f);
        d.uv      = glm::vec4(s.uv0, s.uv1);
        d.uv2_pad = glm::vec4(s.uv2, 0.0f, 0.0f);
        out.worldBounds.expand(s.bounds);
    }

    std::fprintf(stderr,
        "[GpuBVHBuilder] %zu tris, %zu nodes\n",
        out.tris.size(), out.nodes.size());
    return out;
}

} // namespace GpuBVHBuilder
