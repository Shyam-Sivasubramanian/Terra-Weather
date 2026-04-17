#include <glm/glm.hpp>
#pragma once

#include "WorldData.h"

/**
 * @brief Wind field generation
 */
class WindField {
public:
    /**
     * @brief Create wind field generator
     */
    explicit WindField(WorldData& world);

    /**
     * @brief Generate wind field based on terrain
     */
    void generate();

    /**
     * @brief Smooth wind field
     */
    void smoothWind();

    /**
     * @brief Get wind velocity at position
     */
    glm::vec2 getWind(float x, float z) const {
        int gx = static_cast<int>(x * world.width);
        int gz = static_cast<int>(z * world.height);
        return glm::vec2(world.get(world.windU, gx, gz),
                        world.get(world.windV, gx, gz));
    }

private:
    WorldData& world;
    float simpleNoise(float x, float y) const;
};
