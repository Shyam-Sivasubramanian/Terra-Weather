#include "Renderer.h"
#include <GLFW/glfw3.h>

namespace Renderer {
    void renderStub(const WorldData& worldData) {
        float h = 0.5f;
        if (!worldData.heightMap.empty()) {
            int cx = worldData.width / 2;
            int cz = worldData.height / 2;
            h = worldData.get(worldData.heightMap, cx, cz);
        }

        glClearColor(0.2f + h * 0.3f, 0.3f + h * 0.2f, 0.5f + h * 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}