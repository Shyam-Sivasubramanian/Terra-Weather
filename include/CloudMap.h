#pragma once

struct WorldData;

namespace CloudMap {

// Build WorldData.cloudDensity as a 3D grid [width x cloudLayers x height].
void build(WorldData& world);

// Trilinear density sample at world-space position (px, py, pz).
// py is world-space Y (not normalized).
float getCloudDensity(const WorldData& world, float px, float py, float pz);

// Advect the density grid along windU/windV by dt seconds.
// Simple semi-Lagrangian in the XZ plane; keeps grid resolution constant.
void advect(WorldData& world, float dt);

} // namespace CloudMap
