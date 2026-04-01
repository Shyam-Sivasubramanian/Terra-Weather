
#include <iostream>
#include "WorldData.h"
#include "Window.h"
#include "HeightMap.h"
#include "Renderer.h"

int main() {
    Window window;
    if (!window.init(1280, 720, "ProceduralWorld")) {
        return 1;
    }

    WorldData worldData;
    worldData.width = 256;
    worldData.height = 256;
    worldData.seed = 42;

    HeightMap::build(worldData);

    std::cout << "Height map generated with "
              << worldData.heightMap.size()
              << " cells.\n";

    while (!window.shouldClose()) {
        window.pollEvents();
        Renderer::renderStub(worldData);
        window.swapBuffers();
    }

    return 0;
}