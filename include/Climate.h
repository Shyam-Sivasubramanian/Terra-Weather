#pragma once

struct WorldData;

namespace HumidityMap  { void build(WorldData& world); }
namespace WindField    { void build(WorldData& world); }
namespace WeatherMap   { void build(WorldData& world); }

namespace Precipitation {
    void update(WorldData& world, float dt);
    float getIntensity(const WorldData& world, int x, int z);
}
