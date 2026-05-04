#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Hittable.h"

struct WorldData;
class Material;

// One triangle. Minimal data; material pointer indexes into scene-owned materials.
struct Tri {
    glm::vec3 v0, v1, v2;
    glm::vec3 n0, n1, n2;   // per-vertex normals for smooth shading
    glm::vec2 uv0, uv1, uv2;
    const Material* mat = nullptr;
    AABB bounds;
};

// A chunk of contiguous terrain triangles (one cell block) that behaves as
// a leaf in the scene BVH. Keeping chunks coarser than single triangles means
// the BVH stays small enough to build in seconds on a 256x256 grid.
class TerrainChunk : public Hittable {
public:
    TerrainChunk(std::vector<Tri> tris);

    bool hit(const Ray& r, float tmin, float tmax, HitRecord& rec) const override;
    bool boundingBox(AABB& out) const override;

private:
    std::vector<Tri> tris_;
    AABB box_;
};

// Factory: build a list of TerrainChunks covering the full heightmap.
// Each chunk covers a (chunkSize x chunkSize) block of cells and contains
// 2 triangles per cell. Water triangles below seaLevel are tagged with waterMat.
namespace TerrainBuilder {
    std::vector<std::shared_ptr<Hittable>> build(
        const WorldData& world,
        const Material* terrainMat,
        const Material* waterMat,
        int chunkSize = 16);
}
