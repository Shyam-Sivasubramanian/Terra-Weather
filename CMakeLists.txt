cmake_minimum_required(VERSION 3.20)
project(ProceduralWorld)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# Include directories
include_directories(include)
include_directories(src)
include_directories(src/core)
include_directories(src/terrain)
include_directories(src/climate)
include_directories(src/raytracer)

# Source files
set(SOURCES
    src/main.cpp
    src/core/Window.cpp
    src/core/Framebuffer.cpp
    src/core/Camera.cpp
    src/terrain/HeightMap.cpp
    src/terrain/noisegen.cpp
    src/climate/HumidityMap.cpp
    src/climate/WindField.cpp
    src/climate/weathermap.cpp
    src/climate/Precipitation.cpp
    src/raytracer/Renderer.cpp
    src/raytracer/Scene.cpp
    src/raytracer/RayTrace.cpp
    src/raytracer/BVH.cpp
    src/raytracer/TerrainHittable.cpp
    src/raytracer/Atmosphere.cpp
    src/raytracer/VolumetricCloud.cpp
    src/raytracer/WeatherVolume.cpp
    src/raytracer/Materials.cpp
)

add_executable(ProceduralWorld ${SOURCES})

target_link_libraries(ProceduralWorld
    OpenGL::GL
    glfw
    GLEW::GLEW
    pthread
)
