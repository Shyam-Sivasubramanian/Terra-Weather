#!/bin/bash
cd "/mnt/c/Users/shyam/Documents/code/Terra Weather/Terra-Weather"

# 1. Clean up broken glad headers so CMake uses external/glad natively
rm -rf include/glad include/KHR

# 2. Fix the lacunarity variable missing in terrain gen
sed -i 's/frequency \*= lacunarity;/frequency *= 2.0f;/g' src/terrain/noisegen.cpp

# 3. Clean up Camera struct naming mismatches 
find src/ include/ -type f -exec sed -i 's/DOFCamera/Camera/g' {} + 2>/dev/null
sed -i 's/Renderer::Camera/Camera/g' src/main.cpp 2>/dev/null
sed -i 's/\.fov /.vfov /g' src/main.cpp 2>/dev/null

# 4. Strip Window.cpp of the manual GL loader lines that conflict with Glad
sed -i '/getGLProcAddress/d' src/core/Window.cpp
sed -i '/typedef .*PFNGL/d' src/core/Window.cpp
sed -i '/static PFNGL/d' src/core/Window.cpp

# 5. Remove duplicate Window methods (from my previous failed patches)
sed -i '/bool Window::isKeyPressed/d' src/core/Window.cpp 2>/dev/null
sed -i '/void Window::display/d' src/core/Window.cpp 2>/dev/null
sed -i '/void Window::init/d' src/core/Window.cpp 2>/dev/null
sed -i '/bool isKeyPressed/d' include/Window.h 2>/dev/null
sed -i '/void display/d' include/Window.h 2>/dev/null
sed -i '/void init(int/d' include/Window.h 2>/dev/null

# 6. Add Window methods cleanly
sed -i '/class Window {/a \public:\n    bool isKeyPressed(int key) const;\n    void display(const float* fb, int w, int h);\n    void init(int w, int h);' include/Window.h

sed -i '1i #include <GLFW/glfw3.h>' src/core/Window.cpp

cat << 'INNER_EOF' >> src/core/Window.cpp
bool Window::isKeyPressed(int key) const { return glfwGetKey(glfwGetCurrentContext(), key) == GLFW_PRESS; }
void Window::display(const float* fb, int w, int h) {
    if(fb) { glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_FLOAT, fb); }
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glfwSwapBuffers(glfwGetCurrentContext());
}
void Window::init(int w, int h) {}
INNER_EOF

# 7. Ensure WorldData has the 2-argument getCloudDensity needed by RayTrace.cpp
if ! grep -q "getCloudDensity(int" include/WorldData.h; then
    sed -i '/std::vector<float> cloudDensity;/a \    float getCloudDensity(int x, int z) const { return cloudDensity.empty() ? 0.0f : cloudDensity[0]; }' include/WorldData.h
fi

# 8. Rebuild the project
cd build
make clean
make -j$(nproc)
