#pragma once

#include "GpuSceneData.h"

struct WorldData;

namespace GpuBVHBuilder {

// Material IDs used by the compute shader's switch.
constexpr int MAT_TERRAIN = 0;
constexpr int MAT_WATER   = 1;

// Build a flat BVH over all terrain triangles (plus water where underwater)
// from the given WorldData. Result is a list of flattened nodes and a
// triangle array where each leaf references a contiguous range.
GpuSceneData build(const WorldData& world);

} // namespace GpuBVHBuilder
