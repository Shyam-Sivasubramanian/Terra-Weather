#include "../core/Camera.h"
#include "../core/Camera.h"
class Camera;
#pragma once

#include "RayTrace.h"
#include "Scene.h"
#include "glm/glm.hpp"
#include <memory>
#include <cstddef>

/**
 * @brief Tile-based parallel ray tracing renderer
 *
 * Implements progressive rendering with:
 * - Tile-based work distribution
 * - Multi-threaded rendering
 * - Sample accumulation
 * - Progressive refinement
 * - Tone mapping and gamma correction
 */
class Renderer {
public:
    /**
     * @brief Construct renderer
     */
    Renderer();
    ~Renderer();

    /**
     * @brief Set scene to render
     */
    void setScene(std::shared_ptr<Scene> scene);

    /**
     * @brief Render frame
     *
     * @param width Framebuffer width
     * @param height Framebuffer height
     */
    void render(int width, int height);

    /**
     * @brief Get current framebuffer data (RGBA float)
     */
    const float* getFramebuffer() const;

    /**
     * @brief Set samples per pixel
     */
    void setSamplesPerPixel(int spp);

    /**
     * @brief Set maximum ray bounces
     */
    void setMaxBounces(int bounces);

    /**
     * @brief Set render resolution (0.25 = 1/4 resolution)
     */
    void setRenderResolution(float resolution);

    /**
     * @brief Set tile size for parallel rendering
     */
    void setTileSize(int size);

    /**
     * @brief Set camera parameters
     */
    void setCamera(const Camera& cam);

    /**
     * @brief Check if currently rendering
     */
    bool isRendering() const;

    /**
     * @brief Check if frame needs update
     */
    bool needsUpdate() const;

    /**
     * @brief Get render progress (0 to 1)
     */
    float getProgress() const;

    /**
     * @brief Get elapsed render time
     */
    float getElapsedTime() const;

    /**
     * @brief Camera structure
     */
    struct Camera {
        glm::vec3 position;
        glm::vec3 lookAt;
        glm::vec3 up;
        float fov;              // Field of view in degrees
        float aperture;         // Aperture size (0 = no DOF)
        float focusDistance;    // Focus distance
    };

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Progressive renderer with adaptive sampling
 */
class ProgressiveRenderer {
public:
    /**
     * @brief Render until converged or time limit
     */
    void renderUntilConverged(Renderer& renderer, float timeLimit = 5.0f);
};

/**
 * @brief Denoiser for noisy renders
 */
class Denoiser {
public:
    /**
     * @brief Apply temporal denoising
     */
    static void denoiseTemporal(float* framebuffer, int width, int height,
                                const float* previousFrame, float blend);
};
