#pragma once

struct WorldData;

namespace HeightMap {

// Build heightMap from procedural noise; populate seaLevel/snowLevel.
void build(WorldData& world);

// Rebuild using the world's current seed (call on seed change).
void rebuild(WorldData& world, unsigned int seed);

} // namespace HeightMap
