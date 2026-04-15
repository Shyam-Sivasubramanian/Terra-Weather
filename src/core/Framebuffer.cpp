#include "Framebuffer.h"
#include <cstring>

Framebuffer::Framebuffer(int width, int height)
    : width(width), height(height) {
    data.resize(width * height * 4, 0.0f);
}

void Framebuffer::setPixel(int x, int y, const glm::vec3& color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    int idx = (y * width + x) * 4;
    data[idx + 0] = color.r;
    data[idx + 1] = color.g;
    data[idx + 2] = color.b;
    data[idx + 3] = 1.0f;
}

glm::vec3 Framebuffer::getPixel(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return glm::vec3(0.0f);

    int idx = (y * width + x) * 4;
    return glm::vec3(data[idx], data[idx + 1], data[idx + 2]);
}

void Framebuffer::clear(const glm::vec3& color) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            setPixel(x, y, color);
        }
    }
}

void Framebuffer::resize(int w, int h) {
    width = w;
    height = h;
    data.resize(w * h * 4, 0.0f);
}

const float* Framebuffer::getData() const {
    return data.data();
}

int Framebuffer::getWidth() const { return width; }
int Framebuffer::getHeight() const { return height; }
