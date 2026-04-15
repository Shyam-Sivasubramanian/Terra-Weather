cmake_minimum_required(VERSION 3.20)
project(ProceduralWorld)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(include)

add_executable(ProceduralWorld
    src/main.cpp
    src/core/Window.cpp
    src/terrain/HeightMap.cpp
    src/raytracer/Renderer.cpp
)

target_link_libraries(ProceduralWorld
    OpenGL::GL
    glfw
)