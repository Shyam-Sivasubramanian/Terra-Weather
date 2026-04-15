#pragma once

#include "WorldData.h"

/**
 * @brief Precipitation simulation
 */
class Precipitation {
public:
    /**
     * @brief Create precipitation simulator
     */
    explicit Precipitation(WorldData& world);

    /**
     * @brief Update precipitation state
     */
    void update(float deltaTime);

    /**
     * @brief Apply evaporation
     */
    void applyEvaporation(float rate);

private:
    WorldData& world;
};
