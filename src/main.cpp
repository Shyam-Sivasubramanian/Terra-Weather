#include "WorldData.h"
#include "Window.h"
#include "Framebuffer.h"
#include "Camera.h"
#include "Scene.h"
#include "Atmosphere.h"
#include "VolumetricCloud.h"
#include "WeatherVolume.h"
#include "Renderer.h"
#include "RayTrace.h"
#include "HeightMap.h"
#include "Climate.h"
#include "CloudMap.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <chrono>
#include <string>
#include <cstdlib>

// Low-res preview dimensions; refined to window size progressively.
static constexpr int kWorldGrid   = 192;
static constexpr int kWinW        = 1280;
static constexpr int kWinH        = 720;
static constexpr float kPreviewScale = 0.25f; // 1/4 resolution preview

static void printUsage() {
    std::puts(
        "ProceduralWorld controls:\n"
        "  WASD        move horizontally (shift = sprint)\n"
        "  Q/E         move down/up\n"
        "  RMB + drag  look around\n"
        "  R           regenerate world with a new seed\n"
        "  [ / ]       time of day earlier / later\n"
        "  F12         save screenshot (full-res) to screenshot.png\n"
        "  ESC         quit\n");
}

static void buildAllWorldData(WorldData& world) {
    HeightMap::build(world);
    HumidityMap::build(world);
    WindField::build(world);
    WeatherMap::build(world);
    CloudMap::build(world);
}

int main(int argc, char** argv) {
    printUsage();

    // --- World setup ---
    WorldData world;
    world.seed = 42;
    world.resizeAll(kWorldGrid, kWorldGrid);
    world.worldScale  = 200.0f;
    world.heightScale = 40.0f;
    buildAllWorldData(world);

    // --- Window ---
    Window win;
    if (!win.init(kWinW, kWinH, "ProceduralWorld — Ray Tracer")) return 1;

    // --- Framebuffer (starts at preview resolution) ---
    int fbW = static_cast<int>(kWinW * kPreviewScale);
    int fbH = static_cast<int>(kWinH * kPreviewScale);
    Framebuffer fb;
    fb.resize(fbW, fbH);

    // --- Camera ---
    Camera cam;
    glm::vec3 camPos(0.0f, world.heightScale * 0.9f, world.worldScale * 0.6f);
    glm::vec3 camLookAt(0.0f, world.heightScale * 0.3f, 0.0f);
    cam.init(camPos, camLookAt, glm::vec3(0, 1, 0),
             60.0f, static_cast<float>(fbW) / fbH, /*aperture*/ 0.0f, /*focus*/ 50.0f);

    // --- Scene ---
    Scene scene;
    scene.build(world,
                "assets/textures/grass.png",
                "assets/textures/rock.png",
                "assets/textures/snow.png");

    // --- Atmosphere + volumes ---
    Atmosphere atmosphere;
    atmosphere.setSunFromTimeOfDay(0.40f);
    VolumetricCloud clouds;
    WeatherVolume   weather;

    // --- Render context + renderer ---
    RayTrace::Context ctx;
    ctx.scene      = &scene;
    ctx.atmosphere = &atmosphere;
    ctx.clouds     = &clouds;
    ctx.weather    = &weather;
    ctx.world      = &world;
    ctx.maxDepth   = 4;  // preview-friendly

    Renderer renderer;
    renderer.init(&fb, &cam, ctx, /*tileSize*/ 32, /*threads*/ 0);

    bool     previewMode = true;   // low-res, 1 SPP
    int      ssp         = 1;      // samples per accumulation pass
    float    timeOfDay   = 0.40f;

    auto lastFrame = std::chrono::steady_clock::now();

    // Edge-detected keys
    bool prevR = false, prevLB = false, prevRB = false, prevF12 = false;

    while (!win.shouldClose()) {
        win.pollEvents();

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastFrame).count();
        lastFrame = now;

        GLFWwindow* wh = win.handle();

        // --- Input ---
        if (glfwGetKey(wh, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        bool cameraMoved = cam.update(wh, dt);

        bool R_now = glfwGetKey(wh, GLFW_KEY_R) == GLFW_PRESS;
        if (R_now && !prevR) {
            world.seed = static_cast<unsigned int>(std::rand());
            buildAllWorldData(world);
            scene.build(world,
                        "assets/textures/grass.png",
                        "assets/textures/rock.png",
                        "assets/textures/snow.png");
            std::printf("[main] rebuilt world with seed=%u\n", world.seed);
            cameraMoved = true;
        }
        prevR = R_now;

        bool LB_now = glfwGetKey(wh, GLFW_KEY_LEFT_BRACKET)  == GLFW_PRESS;
        bool RB_now = glfwGetKey(wh, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS;
        if (LB_now && !prevLB) {
            timeOfDay = std::max(0.0f, timeOfDay - 0.05f);
            atmosphere.setSunFromTimeOfDay(timeOfDay);
            std::printf("[main] time of day = %.2f\n", timeOfDay);
            cameraMoved = true;
        }
        if (RB_now && !prevRB) {
            timeOfDay = std::min(1.0f, timeOfDay + 0.05f);
            atmosphere.setSunFromTimeOfDay(timeOfDay);
            std::printf("[main] time of day = %.2f\n", timeOfDay);
            cameraMoved = true;
        }
        prevLB = LB_now;
        prevRB = RB_now;

        bool F12_now = glfwGetKey(wh, GLFW_KEY_F12) == GLFW_PRESS;
        if (F12_now && !prevF12) {
            if (fb.savePNG("screenshot.png")) {
                std::puts("[main] saved screenshot.png");
            } else {
                std::puts("[main] screenshot failed");
            }
        }
        prevF12 = F12_now;

        // --- Window resize handling ---
        int newW, newH;
        if (win.consumeResized(newW, newH)) {
            fbW = std::max(1, static_cast<int>(newW * (previewMode ? kPreviewScale : 1.0f)));
            fbH = std::max(1, static_cast<int>(newH * (previewMode ? kPreviewScale : 1.0f)));
            fb.resize(fbW, fbH);
            cam.setAspect(static_cast<float>(fbW) / fbH);
            cameraMoved = true;
        }

        // --- Progressive rendering strategy ---
        // On camera move: reset accumulator, drop to preview resolution, 1 SPP.
        // When idle: increase SPP; when SPP > threshold, step up to full res.
        if (cameraMoved) {
            fb.reset();
            previewMode = true;
            if (fb.width() != static_cast<int>(win.width() * kPreviewScale) ||
                fb.height() != static_cast<int>(win.height() * kPreviewScale)) {
                fb.resize(static_cast<int>(win.width() * kPreviewScale),
                          static_cast<int>(win.height() * kPreviewScale));
                cam.setAspect(static_cast<float>(fb.width()) / fb.height());
            }
            ssp = 1;
        }

        // Promote preview to full res after enough samples have accumulated.
        int minS = fb.minSampleCount();
        if (previewMode && minS >= 4) {
            previewMode = false;
            fb.resize(win.width(), win.height());
            cam.setAspect(static_cast<float>(win.width()) / win.height());
            fb.reset();
            ssp = 1;
        }

        // --- Trace one pass ---
        renderer.renderTiles(ssp);

        // --- Blit framebuffer to window ---
        win.blit(fb);
        win.swap();
    }

    return 0;
}
