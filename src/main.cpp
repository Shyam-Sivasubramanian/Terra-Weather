#include "core/Camera.h"
#include "core/Camera.h"
#include <GLFW/glfw3.h>
#include "core/Camera.h"
#include <GLFW/glfw3.h>
#include "core/Camera.h"
#include "core/Window.h"
#include "Framebuffer.h"
#include "core/Camera.h"
#include "WorldData.h"
#include "raytracer/Scene.h"
#include "raytracer/Renderer.h"
#include "terrain/NoiseGen.h"
#include "terrain/HeightMap.h"
#include "HumidityMap.h"
#include "WindField.h"
#include "WeatherMap.h"
#include "Atmosphere.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <chrono>

/**
 * @brief Main application class
 */
class Application {
public:
    Application() : width(1280), height(720) {
        // Initialize GLFW
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Create window
        window = std::make_unique<Window>(); window->init(width, height);

        // Initialize world data
        worldData = std::make_shared<WorldData>();
        worldData->width = 512;
        worldData->height = 512;
        worldData->seed = 42;
        worldData->heightMap.resize(worldData->width * worldData->height);
        worldData->humidityMap.resize(worldData->width * worldData->height);
        worldData->temperatureMap.resize(worldData->width * worldData->height);
        worldData->windU.resize(worldData->width * worldData->height);
        worldData->windV.resize(worldData->width * worldData->height);
        worldData->weatherMap.resize(worldData->width * worldData->height);
        worldData->cloudDensity.resize(worldData->width * worldData->height);

        // Generate terrain
        generateTerrain();

        // Generate climate
        generateClimate();

        // Create scene
        scene = std::make_shared<Scene>(*worldData);

        // Create renderer
        renderer = std::make_unique<Renderer>();
        renderer->setScene(scene);
        renderer->setRenderResolution(0.25f);  // Start at 1/4 resolution
        renderer->setSamplesPerPixel(1);

        // Camera
        camera.position = glm::vec3(0.5f, 0.6f, -1.5f);
        camera.lookAt = glm::vec3(0.5f, 0.4f, 0.5f);
        camera.vfov = 60.0f;
        renderer->setCamera(camera);
    }

    ~Application() {
        glfwTerminate();
    }

    void generateTerrain() {
        // Generate noise-based height map
        NoiseGen noise(worldData->seed);
        HeightMap heightMap(*worldData);

        heightMap.generate(noise);
        std::cout << "Terrain generated: " << worldData->width << "x" << worldData->height << std::endl;
    }

    void generateClimate() {
        // Generate humidity map
        HumidityMap humidityMap(*worldData);
        humidityMap.generate();
        std::cout << "Humidity generated" << std::endl;

        // Generate wind field
        WindField windField(*worldData);
        windField.generate();
        std::cout << "Wind field generated" << std::endl;

        // Generate weather map
        WeatherMap weatherMap(*worldData);
        weatherMap.generate();
        std::cout << "Weather map generated" << std::endl;

        // Generate cloud density
        generateCloudDensity();
    }

    void generateCloudDensity() {
        // Simple cloud generation based on humidity
        for (int z = 0; z < worldData->height; z++) {
            for (int x = 0; x < worldData->width; x++) {
                float humidity = worldData->get(worldData->humidityMap, x, z);
                float height = worldData->get(worldData->heightMap, x, z);

                // Clouds form above certain height if humid
                if (height > 0.3f && humidity > 0.5f) {
                    float cloudChance = (humidity - 0.5f) * 2.0f;
                    worldData->cloudDensity[z * worldData->width + x] = cloudChance * 0.8f;
                }
            }
        }
    }

    void run() {
        std::cout << "Starting render..." << std::endl;

        while (!window->shouldClose()) {
            // Handle input
            handleInput();

            // Render if needed
            if (renderer->needsUpdate()) {
                renderer->render(width, height);
            }

            // Display framebuffer
            const float* fb = renderer->getFramebuffer();
            if (fb) {
                window->display(fb, width, height);
            }

            // Check render progress
            if (renderer->isRendering()) {
                float progress = renderer->getProgress();
                std::cout << "\rRender progress: " << (progress * 100) << "%" << std::flush;
            } else {
                // Increase resolution when idle
                if (renderer->getProgress() >= 1.0f) {
                    static bool upgraded = false;
                    if (!upgraded) {
                        renderer->setRenderResolution(0.5f);
                        upgraded = true;
                        std::cout << "\nUpgraded to 1/2 resolution" << std::endl;
                    }
                }
            }

            glfwPollEvents();
        }

        std::cout << "\nApplication closed." << std::endl;
    }

    void handleInput() {
        // Camera movement
        float moveSpeed = 0.01f;
        bool moved = false;

        if (window->isKeyPressed(GLFW_KEY_W)) {
            glm::vec3 dir = glm::normalize(camera.lookAt - camera.position);
            camera.position += dir * moveSpeed;
            moved = true;
        }
        if (window->isKeyPressed(GLFW_KEY_S)) {
            glm::vec3 dir = glm::normalize(camera.lookAt - camera.position);
            camera.position -= dir * moveSpeed;
            moved = true;
        }
        if (window->isKeyPressed(GLFW_KEY_A)) {
            glm::vec3 dir = glm::normalize(camera.lookAt - camera.position);
            glm::vec3 right = glm::cross(dir, glm::vec3(0, 1, 0));
            camera.position -= right * moveSpeed;
            moved = true;
        }
        if (window->isKeyPressed(GLFW_KEY_D)) {
            glm::vec3 dir = glm::normalize(camera.lookAt - camera.position);
            glm::vec3 right = glm::cross(dir, glm::vec3(0, 1, 0));
            camera.position += right * moveSpeed;
            moved = true;
        }

        // Reset on camera change
        if (moved) {
            camera.lookAt = camera.position + glm::vec3(0, 0, 2);
            renderer->setCamera(camera);
        }
    }

private:
    int width, height;
    std::unique_ptr<Window> window;
    std::shared_ptr<WorldData> worldData;
    std::shared_ptr<Scene> scene;
    std::unique_ptr<Renderer> renderer;
    Camera camera;
};

int main() {
    try {
        std::cout << "Procedural Terrain & Weather Ray Tracer" << std::endl;
        std::cout << "=======================================" << std::endl;

        Application app;
        app.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
