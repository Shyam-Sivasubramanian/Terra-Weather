#pragma once

#include "Ray.h"
#include "Scene.h"
#include "glm/glm.hpp"
#include <memory>
#include "../core/Camera.h"

/**
 * @brief Core ray tracing engine
 *
 * Handles the main ray tracing loop with:
 * - Surface intersection
 * - Material scattering
 * - Russian roulette termination
 * - Volume integration
 */
class RayTracer {
public:
    /**
     * @brief Construct ray tracer with scene
     */
    explicit RayTracer(Scene& scene);
    ~RayTracer();

    /**
     * @brief Trace a single ray
     *
     * @param ray The ray to trace
     * @param time Current time for animated scenes
     * @return Accumulated color along the ray path
     */
    glm::vec3 trace(const Ray& ray, float time = 0.0f);

    /**
     * @brief Trace with explicit volume integration
     */
    glm::vec3 traceWithVolume(const Ray& ray, float time = 0.0f);

    /**
     * @brief Set maximum ray bounce depth
     */
    void setMaxBounces(int bounces);

    /**
     * @brief Set shadow ray count (for soft shadows)
     */
    void setShadowRays(int rays);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

/**
 * @brief Ray generation parameters
 */
struct CameraParams {
    glm::vec3 origin;           // Camera position
    glm::vec3 lookAt;           // Look target
    glm::vec3 up;               // Up vector
    float fov;                  // Field of view in degrees
    float aspectRatio;          // Width / Height
    float aperture;             // Aperture size for DOF
    float focusDistance;        // Focus distance for DOF
    float time0, time1;         // Shutter open/close times for motion blur
};

/**
 * @brief Generate camera ray for a given pixel
 */
Ray generateCameraRay(const CameraParams& params, float u, float v,
                     float uJitter = 0.0f, float vJitter = 0.0f);

/**
 * @brief Camera with depth of field support
 */

private:
    static glm::vec3 randomInUnitDisk() {
        // Implementation of random point in unit disk
        return glm::vec3(0.0f);
    }
};

/**
 * @brief Monte Carlo path tracer
 */
class PathTracer {
public:
    PathTracer(Scene& scene) : rayTracer(scene) {}

    /**
     * @brief Trace path with MIS (Multiple Importance Sampling)
     */
    glm::vec3 traceMIS(const Ray& ray, int maxDepth);

    /**
     * @brief Next event estimation
     */
    glm::vec3 nextEventEstimation(const HitRecord& hit, const Scene& scene);

private:
    RayTracer rayTracer;
};

/**
 * @brief Bidirectional path tracer
 */
class BidirectionalPathTracer {
public:
    glm::vec3 trace(const Ray& ray);
};
