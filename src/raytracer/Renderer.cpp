#include "Renderer.h"
#include "Framebuffer.h"
#include "Camera.h"
#include "Material.h"  // randFloat

#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <queue>
#include <algorithm>

struct Tile {
    int x0, y0, x1, y1; // exclusive end
};

void Renderer::init(Framebuffer* fb, Camera* cam,
                    const RayTrace::Context& ctx,
                    int tileSize, int threadCount) {
    fb_       = fb;
    cam_      = cam;
    ctx_      = ctx;
    tileSize_ = std::max(4, tileSize);
    unsigned hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 2;
    threads_ = (threadCount > 0) ? threadCount : static_cast<int>(hw);
}

void Renderer::renderTiles(int spp) {
    if (!fb_ || !cam_) return;
    const int W = fb_->width();
    const int H = fb_->height();
    if (W <= 0 || H <= 0 || spp <= 0) return;

    // Build tile list.
    std::vector<Tile> tiles;
    tiles.reserve((W / tileSize_ + 1) * (H / tileSize_ + 1));
    for (int y = 0; y < H; y += tileSize_) {
        for (int x = 0; x < W; x += tileSize_) {
            Tile t;
            t.x0 = x; t.y0 = y;
            t.x1 = std::min(x + tileSize_, W);
            t.y1 = std::min(y + tileSize_, H);
            tiles.push_back(t);
        }
    }
    if (tiles.empty()) return;

    std::atomic<std::size_t> nextTile{0};
    std::atomic<std::size_t> doneTiles{0};
    const std::size_t totalTiles = tiles.size();

    auto worker = [&]() {
        while (true) {
            std::size_t idx = nextTile.fetch_add(1);
            if (idx >= totalTiles) break;
            const Tile& t = tiles[idx];

            for (int y = t.y0; y < t.y1; ++y) {
                for (int x = t.x0; x < t.x1; ++x) {
                    glm::vec3 acc(0.0f);
                    for (int s = 0; s < spp; ++s) {
                        // Jitter within pixel for AA.
                        float jx = randFloat();
                        float jy = randFloat();
                        float u = (x + jx) / static_cast<float>(W);
                        float v = (y + jy) / static_cast<float>(H);
                        Ray r = cam_->getRay(u, v);
                        acc += RayTrace::trace(r, ctx_);
                    }
                    // Split into individual samples — framebuffer averages internally.
                    for (int s = 0; s < spp; ++s) {
                        // We only stored the sum; add one sample worth
                        // (sum/spp) repeated spp times is equivalent to
                        // adding the sum with spp increments.
                        fb_->addSample(x, y, acc / static_cast<float>(spp));
                    }
                }
            }
            std::size_t d = doneTiles.fetch_add(1) + 1;
            lastProgress_.store(static_cast<float>(d) / totalTiles);
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(threads_);
    for (int i = 0; i < threads_; ++i) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    lastProgress_.store(1.0f);
}
