#include "CPURenderer.h"
#include "RayTrace.h"
#include "Camera.h"

bool CPURenderer::init(int w, int h, Camera* cam, const RayTrace::Context& ctx,
                       int tileSize, int threadCount) {
    fb_.resize(w, h);
    inner_.init(&fb_, cam, ctx, tileSize, threadCount);
    return true;
}
