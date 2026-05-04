#pragma once

#include <glm/glm.hpp>

class Material;
class Texture2D;

namespace Materials {
    Material* makeLambertian(const glm::vec3& albedo);
    Material* makeReflective(const glm::vec3& albedo, float roughness);
    Material* makeTerrain(const Texture2D* grass,
                          const Texture2D* rock,
                          const Texture2D* snow,
                          float snowLevelWorldY,
                          float heightScale);
}
