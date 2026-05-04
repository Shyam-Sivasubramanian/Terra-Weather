#include "TerrainHittable.h"
#include "WorldData.h"
#include "Material.h"

#include <algorithm>
#include <cmath>

TerrainChunk::TerrainChunk(std::vector<Tri> tris)
    : tris_(std::move(tris)) {
    for (const auto& t : tris_) {
        box_.expand(t.v0);
        box_.expand(t.v1);
        box_.expand(t.v2);
    }
    // Guard against zero-volume boxes (flat chunks) that break slab test.
    const float eps = 1e-3f;
    for (int a = 0; a < 3; ++a) {
        if (box_.max[a] - box_.min[a] < eps) {
            box_.min[a] -= eps;
            box_.max[a] += eps;
        }
    }
}

bool TerrainChunk::boundingBox(AABB& out) const {
    out = box_;
    return true;
}

// Möller-Trumbore ray-triangle intersection.
// Returns (tHit, u, v) via out params if hit.
static bool rayTriangle(const Ray& r, const Tri& t,
                        float tmin, float tmax,
                        float& tHit, float& u, float& v) {
    const glm::vec3 e1 = t.v1 - t.v0;
    const glm::vec3 e2 = t.v2 - t.v0;
    const glm::vec3 p  = glm::cross(r.direction, e2);
    float det = glm::dot(e1, p);
    // Two-sided: accept both faces (we cull nothing here to keep it simple).
    if (std::fabs(det) < 1e-8f) return false;
    float invDet = 1.0f / det;

    glm::vec3 s = r.origin - t.v0;
    u = glm::dot(s, p) * invDet;
    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, e1);
    v = glm::dot(r.direction, q) * invDet;
    if (v < 0.0f || u + v > 1.0f) return false;

    float th = glm::dot(e2, q) * invDet;
    if (th < tmin || th > tmax) return false;

    tHit = th;
    return true;
}

bool TerrainChunk::hit(const Ray& r, float tmin, float tmax, HitRecord& rec) const {
    // Quick reject against chunk's own AABB (BVH already did this for the chunk
    // but not for inner triangles — cheap extra prune).
    if (!box_.hit(r, tmin, tmax)) return false;

    bool found = false;
    float closest = tmax;
    const Tri* bestTri = nullptr;
    float bestU = 0.0f, bestV = 0.0f;

    for (const auto& t : tris_) {
        float th, u, v;
        if (rayTriangle(r, t, tmin, closest, th, u, v)) {
            closest = th;
            bestTri = &t;
            bestU = u; bestV = v;
            found = true;
        }
    }

    if (!found) return false;

    const Tri& t = *bestTri;
    float w = 1.0f - bestU - bestV;
    rec.t = closest;
    rec.point = r.at(closest);
    // Barycentric-interpolate smooth normal + UV.
    glm::vec3 n = glm::normalize(w * t.n0 + bestU * t.n1 + bestV * t.n2);
    rec.setFaceNormal(r, n);
    rec.uv  = w * t.uv0 + bestU * t.uv1 + bestV * t.uv2;
    rec.mat = t.mat;
    return true;
}

// ----------------------------------------------------------------------------
// Build helpers
// ----------------------------------------------------------------------------
namespace {

// Map grid (x, z) to world-space position. The terrain spans
// [-worldScale/2, +worldScale/2] on X and Z. Y is heightMap * heightScale.
glm::vec3 gridToWorld(const WorldData& w, int gx, int gz) {
    float fx = static_cast<float>(gx) / std::max(1, w.width  - 1);
    float fz = static_cast<float>(gz) / std::max(1, w.height - 1);
    float x = (fx - 0.5f) * w.worldScale;
    float z = (fz - 0.5f) * w.worldScale;
    float hVal = w.heightMap.empty() ? 0.0f
               : w.heightMap[static_cast<std::size_t>(gz) * w.width + gx];
    float y = hVal * w.heightScale;
    return glm::vec3(x, y, z);
}

// Compute per-vertex smoothed normals once, cache them in a flat array.
std::vector<glm::vec3> computeVertexNormals(const WorldData& w) {
    std::vector<glm::vec3> normals(
        static_cast<std::size_t>(w.width) * w.height, glm::vec3(0.0f));

    // For each cell, sum two triangle normals into the 4 corner vertices.
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
            normals[idx(x,     z    )] += n1;
            normals[idx(x + 1, z    )] += n1 + n2;
            normals[idx(x,     z + 1)] += n1 + n2;
            normals[idx(x + 1, z + 1)] += n2;
        }
    }

    for (auto& n : normals) {
        float len2 = glm::dot(n, n);
        n = (len2 > 1e-12f) ? n / std::sqrt(len2) : glm::vec3(0, 1, 0);
    }
    return normals;
}

} // namespace

namespace TerrainBuilder {

std::vector<std::shared_ptr<Hittable>> build(const WorldData& world,
                                             const Material* terrainMat,
                                             const Material* waterMat,
                                             int chunkSize) {
    std::vector<std::shared_ptr<Hittable>> out;
    if (world.width < 2 || world.height < 2) return out;

    auto normals = computeVertexNormals(world);
    const float seaY = world.seaLevel * world.heightScale;

    const int cellsX = world.width  - 1;
    const int cellsZ = world.height - 1;

    for (int cz = 0; cz < cellsZ; cz += chunkSize) {
        for (int cx = 0; cx < cellsX; cx += chunkSize) {
            int ex = std::min(cx + chunkSize, cellsX);
            int ez = std::min(cz + chunkSize, cellsZ);
            std::vector<Tri> tris;
            tris.reserve(static_cast<std::size_t>((ex - cx) * (ez - cz) * 2));

            for (int z = cz; z < ez; ++z) {
                for (int x = cx; x < ex; ++x) {
                    glm::vec3 p00 = gridToWorld(world, x,     z    );
                    glm::vec3 p10 = gridToWorld(world, x + 1, z    );
                    glm::vec3 p01 = gridToWorld(world, x,     z + 1);
                    glm::vec3 p11 = gridToWorld(world, x + 1, z + 1);

                    auto nidx = [&](int xi, int zi) {
                        return normals[static_cast<std::size_t>(zi) * world.width + xi];
                    };

                    // If the cell average is below seaLevel, lift triangles to seaLevel
                    // and tag them as water — this keeps the water plane coincident with
                    // the ocean floor's extent without needing an infinite plane.
                    float avgY = 0.25f * (p00.y + p10.y + p01.y + p11.y);
                    bool underwater = avgY < seaY;
                    const Material* mat = underwater ? waterMat : terrainMat;

                    if (underwater) {
                        p00.y = p10.y = p01.y = p11.y = seaY;
                    }

                    glm::vec2 uv00(static_cast<float>(x)     / (world.width  - 1),
                                   static_cast<float>(z)     / (world.height - 1));
                    glm::vec2 uv10(static_cast<float>(x + 1) / (world.width  - 1),
                                   static_cast<float>(z)     / (world.height - 1));
                    glm::vec2 uv01(static_cast<float>(x)     / (world.width  - 1),
                                   static_cast<float>(z + 1) / (world.height - 1));
                    glm::vec2 uv11(static_cast<float>(x + 1) / (world.width  - 1),
                                   static_cast<float>(z + 1) / (world.height - 1));

                    glm::vec3 flatNormal = underwater ? glm::vec3(0, 1, 0) : glm::vec3(0);

                    // Tri 1: p00, p10, p11 (CCW when viewed from above)
                    Tri t1;
                    t1.v0 = p00; t1.v1 = p10; t1.v2 = p11;
                    t1.n0 = underwater ? flatNormal : nidx(x,     z    );
                    t1.n1 = underwater ? flatNormal : nidx(x + 1, z    );
                    t1.n2 = underwater ? flatNormal : nidx(x + 1, z + 1);
                    t1.uv0 = uv00; t1.uv1 = uv10; t1.uv2 = uv11;
                    t1.mat = mat;
                    tris.push_back(t1);

                    // Tri 2: p00, p11, p01
                    Tri t2;
                    t2.v0 = p00; t2.v1 = p11; t2.v2 = p01;
                    t2.n0 = underwater ? flatNormal : nidx(x,     z    );
                    t2.n1 = underwater ? flatNormal : nidx(x + 1, z + 1);
                    t2.n2 = underwater ? flatNormal : nidx(x,     z + 1);
                    t2.uv0 = uv00; t2.uv1 = uv11; t2.uv2 = uv01;
                    t2.mat = mat;
                    tris.push_back(t2);
                }
            }

            if (!tris.empty()) {
                out.push_back(std::make_shared<TerrainChunk>(std::move(tris)));
            }
        }
    }
    return out;
}

} // namespace TerrainBuilder
