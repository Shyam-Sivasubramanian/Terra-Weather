#include <new>
#include "../core/Camera.h"
#include "../core/Camera.h"
#include <new>
#include "../core/Camera.h"
#include "Renderer.h"
#include "Scene.h"
#include "RayTrace.h"
#include "Atmosphere.h"
#include "VolumetricCloud.h"
#include "WeatherVolume.h"
#include <algorithm>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>

/**
 * @brief Render tile for parallel processing
 */
struct RenderTile {
    int x, y;           // Tile origin in pixels
    int width, height;  // Tile size
    int tileId;         // Unique tile identifier
    bool complete;      // Completion flag
};

/**
 * @brief Render statistics
 */
struct RenderStats {
    std::atomic<int> tilesRendered{0};
    std::atomic<int> totalTiles{0};
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;

    float elapsedSeconds() const {
        auto end = endTime;
        if (tilesRendered < totalTiles) {
            end = std::chrono::high_resolution_clock::now();
        }
        return std::chrono::duration<float>(end - startTime).count();
    }

    float progress() const {
        if (totalTiles == 0) return 0.0f;
        return static_cast<float>(tilesRendered) / totalTiles;
    }
};

/**
 * @brief Thread-safe tile queue
 */
class TileQueue {
public:
    TileQueue(int width, int height, int tileSize = 32)
        : tileSize(tileSize), currentIndex(0) {
        // Calculate number of tiles
        tilesX = (width + tileSize - 1) / tileSize;
        tilesY = (height + tileSize - 1) / tileSize;
        totalTiles = tilesX * tilesY;
    }

    bool getNextTile(RenderTile& tile) {
        std::lock_guard<std::mutex> lock(mutex);
        if (currentIndex >= totalTiles) return false;

        int idx = currentIndex++;
        int tx = idx % tilesX;
        int ty = idx / tilesX;

        tile.x = tx * tileSize;
        tile.y = ty * tileSize;
        tile.width = std::min(tileSize, width - tile.x);
        tile.height = std::min(tileSize, height - tile.y);
        tile.tileId = idx;
        tile.complete = false;

        return true;
    }

    int width, height;
    int tileSize;
    int tilesX, tilesY;
    int totalTiles;

private:
    std::atomic<int> currentIndex;
    std::mutex mutex;
};

/**
 * @brief Framebuffer for accumulation
 */
class Framebuffer {
public:
    Framebuffer(int w, int h, int maxSamples)
        : width(w), height(h), maxSamples(maxSamples) {
        // Accumulation buffer
        accumR.resize(w * h * maxSamples, 0.0f);
        accumG.resize(w * h * maxSamples, 0.0f);
        accumB.resize(w * h * maxSamples, 0.0f);

        // Display buffer
        display.resize(w * h * 4, 0.0f);

        // Sample counts
        sampleCount.resize(w * h, 0);
    }

    void addSample(int x, int y, const glm::vec3& color, int sampleIndex) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        if (sampleIndex >= maxSamples) return;

        int pixelIndex = y * width + x;
        int sampleIdx = pixelIndex * maxSamples + sampleIndex;

        if (sampleIndex == 0) {
            // First sample, just store
            accumR[sampleIdx] = color.r;
            accumG[sampleIdx] = color.g;
            accumB[sampleIdx] = color.b;
        } else {
            // Average with previous samples
            int count = sampleIndex + 1;
            float weight = 1.0f / count;

            accumR[sampleIdx] = accumR[pixelIndex * maxSamples] * (1.0f - weight) + color.r * weight;
            accumG[sampleIdx] = accumG[pixelIndex * maxSamples] * (1.0f - weight) + color.g * weight;
            accumB[sampleIdx] = accumB[pixelIndex * maxSamples] * (1.0f - weight) + color.b * weight;
        }

        sampleCount[pixelIndex] = sampleIndex + 1;
    }

    void updateDisplay(int sampleIndex) {
        #pragma omp parallel for
        for (int i = 0; i < width * height; i++) {
            int pixelIdx = i;
            int sampleIdx = pixelIdx * maxSamples + std::min(sampleIndex, maxSamples - 1);

            // Tone mapping
            glm::vec3 color(accumR[sampleIdx], accumG[sampleIdx], accumB[sampleIdx]);

            // Simple Reinhard tone mapping
            color = color / (color + glm::vec3(1.0f));

            // Gamma correction
            color = glm::pow(color, glm::vec3(1.0f / 2.2f));

            // Clamp to [0, 1]
            color = glm::clamp(color, 0.0f, 1.0f);

            int displayIdx = i * 4;
            display[displayIdx + 0] = color.r;
            display[displayIdx + 1] = color.g;
            display[displayIdx + 2] = color.b;
            display[displayIdx + 3] = 1.0f;
        }
    }

    const float* getDisplayData() const { return display.data(); }

    void reset() {
        std::fill(accumR.begin(), accumR.end(), 0.0f);
        std::fill(accumG.begin(), accumG.end(), 0.0f);
        std::fill(accumB.begin(), accumB.end(), 0.0f);
        std::fill(sampleCount.begin(), sampleCount.end(), 0);
    }

    int width, height;
    int maxSamples;

private:
    std::vector<float> accumR, accumG, accumB;
    std::vector<float> display;
    std::vector<int> sampleCount;
};

/**
 * @brief Renderer Implementation
 */
class Renderer::Impl {
public:
    Impl()
        : running(false), paused(false), needsRedraw(true),
          samplesPerPixel(1), maxBounces(8), tileSize(32),
          renderResolution(1.0f), frameIndex(0) {
        // Get hardware concurrency
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;
    }

    void setScene(std::shared_ptr<Scene> scene_) {
        scene = scene_;
        rayTracer = std::make_unique<RayTracer>(*scene);
        needsRedraw = true;
    }

    void render(int width, int height) {
        if (!scene || !rayTracer) return;

        running = true;
        needsRedraw = false;
        frameIndex++;

        // Initialize framebuffer
        if (!framebuffer || framebuffer->width != width || framebuffer->height != height) {
            framebuffer = std::make_unique<Framebuffer>(width, height, samplesPerPixel);
        }

        // Reset on camera movement
        if (cameraChanged) {
            framebuffer->reset();
            cameraChanged = false;
            frameIndex = 0;
        }

        // Calculate render resolution
        int renderWidth = static_cast<int>(width * renderResolution);
        int renderHeight = static_cast<int>(height * renderResolution);

        // Create tile queue
        TileQueue tileQueue(renderWidth, renderHeight, tileSize);
        stats.~RenderStats(); new (&stats) RenderStats();
        stats.totalTiles = tileQueue.totalTiles;
        stats.startTime = std::chrono::high_resolution_clock::now();

        // Camera parameters
        CameraParams camParams;
        camParams.origin = camera.position;
        camParams.lookAt = camera.lookAt;
        camParams.up = glm::vec3(0.0f, 1.0f, 0.0f);
        camParams.fov = camera.fov;
        camParams.aspectRatio = static_cast<float>(renderWidth) / renderHeight;
        camParams.aperture = camera.aperture;
        camParams.focusDistance = camera.focusDistance;

        // Render tiles in parallel
        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; t++) {
            threads.emplace_back([this, &tileQueue, camParams, renderWidth, renderHeight]() {
                renderTiles(tileQueue, camParams, renderWidth, renderHeight);
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        stats.endTime = std::chrono::high_resolution_clock::now();

        // Update display buffer
        framebuffer->updateDisplay(frameIndex);

        running = false;
    }

    void renderTiles(TileQueue& queue, const CameraParams& camParams,
                    int renderWidth, int renderHeight) {
        RenderTile tile;

        while (queue.getNextTile(tile)) {
            for (int y = 0; y < tile.height; y++) {
                for (int x = 0; x < tile.width; x++) {
                    int px = tile.x + x;
                    int py = tile.y + y;

                    // Normalized coordinates
                    float u = static_cast<float>(px) / renderWidth;
                    float v = static_cast<float>(py) / renderHeight;

                    // Anti-aliasing jitter
                    static thread_local std::random_device rd;
                    static thread_local std::mt19937 gen(rd());
                    static thread_local std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

                    float uJitter = dist(gen);
                    float vJitter = dist(gen);

                    // Generate camera ray
                    Ray ray = generateCameraRay(camParams, u + 0.5f / renderWidth,
                                                1.0f - v + 0.5f / renderHeight,
                                                uJitter, vJitter);

                    // Trace ray
                    glm::vec3 color = rayTracer->trace(ray, 0.0f);

                    // Store in accumulation buffer
                    framebuffer->addSample(px, py, color, frameIndex);

                    // Scale up to display resolution
                    if (renderResolution < 1.0f) {
                        int scale = static_cast<int>(1.0f / renderResolution);
                        for (int dy = 0; dy < scale && py + dy < framebuffer->height; dy++) {
                            for (int dx = 0; dx < scale && px + dx < framebuffer->width; dx++) {
                                // Will be filled by updateDisplay
                            }
                        }
                    }
                }
            }

            stats.tilesRendered++;
        }
    }

    const float* getFramebuffer() const {
        if (framebuffer) {
            return framebuffer->getDisplayData();
        }
        return nullptr;
    }

    void setSamplesPerPixel(int spp) {
        samplesPerPixel = std::max(1, std::min(256, spp));
        framebuffer.reset();
        frameIndex = 0;
        needsRedraw = true;
    }

    void setMaxBounces(int bounces) {
        maxBounces = std::max(1, std::min(16, bounces));
        rayTracer->setMaxBounces(maxBounces);
        needsRedraw = true;
    }

    void setRenderResolution(float resolution) {
        renderResolution = std::max(0.125f, std::min(1.0f, resolution));
        framebuffer.reset();
        frameIndex = 0;
        needsRedraw = true;
    }

    void setTileSize(int size) {
        tileSize = std::max(8, std::min(128, size));
        needsRedraw = true;
    }

    void setCamera(const Camera& cam) {
        camera.position = cam.position; camera.lookAt = cam.lookAt; camera.up = cam.up; camera.vfov = cam.vfov; camera.aperture = cam.aperture; camera.focusDist = cam.focusDist;
        cameraChanged = true;
        needsRedraw = true;
    }

    bool isRendering() const { return running; }
    bool needsUpdate() const { return needsRedraw; }
    float getProgress() const { return stats.progress(); }
    float getElapsedTime() const { return stats.elapsedSeconds(); }


    std::shared_ptr<Scene> scene;
    std::unique_ptr<RayTracer> rayTracer;
    std::unique_ptr<Framebuffer> framebuffer;
    RenderStats stats;

    std::atomic<bool> running;
    std::atomic<bool> paused;
    std::atomic<bool> needsRedraw;
    std::atomic<bool> cameraChanged;

    int samplesPerPixel;
    int maxBounces;
    int tileSize;
    float renderResolution;
    int frameIndex;

    int numThreads;
    Camera camera;
};

/**
 * @brief Renderer Implementation
 */
Renderer::Renderer() : impl(std::make_unique<Impl>()) {}
Renderer::~Renderer() = default;

void Renderer::setScene(std::shared_ptr<Scene> scene) {
    impl->setScene(scene);
}

void Renderer::render(int width, int height) {
    impl->render(width, height);
}

const float* Renderer::getFramebuffer() const {
    return impl->getFramebuffer();
}

void Renderer::setSamplesPerPixel(int spp) {
    impl->setSamplesPerPixel(spp);
}

void Renderer::setMaxBounces(int bounces) {
    impl->setMaxBounces(bounces);
}

void Renderer::setRenderResolution(float resolution) {
    impl->setRenderResolution(resolution);
}

void Renderer::setTileSize(int size) {
    impl->setTileSize(size);
}

void Renderer::setCamera(const Camera& cam) {
    impl->setCamera(cam);
}

bool Renderer::isRendering() const {
    return impl->isRendering();
}

bool Renderer::needsUpdate() const {
    return impl->needsUpdate();
}

float Renderer::getProgress() const {
    return impl->getProgress();
}

float Renderer::getElapsedTime() const {
    return impl->getElapsedTime();
}
