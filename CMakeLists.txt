cmake_minimum_required(VERSION 3.20)
project(ProceduralWorld)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(OpenMP) # for optional OMP fallback

add_subdirectory(external/glfw)
add_subdirectory(external/glad)
add_subdirectory(external/glm)
add_subdirectory(external/imgui)

add_executable(ProceduralWorld
    src/main.cpp
    src/core/Window.cpp
    src/core/Framebuffer.cpp
    src/core/Camera.cpp
    src/terrain/NoiseGen.cpp
    src/terrain/HeightMap.cpp
    src/raytracer/Renderer.cpp
    src/raytracer/Scene.cpp
    src/raytracer/BVH.cpp
    src/raytracer/TerrainHittable.cpp
    src/raytracer/RayTrace.cpp
    src/raytracer/Materials.cpp
    src/raytracer/Atmosphere.cpp
    src/raytracer/VolumetricCloud.cpp
    src/raytracer/WeatherVolume.cpp
    src/climate/HumidityMap.cpp
    src/climate/WindField.cpp
    src/climate/WeatherMap.cpp
    src/climate/Precipitation.cpp
)

target_include_directories(ProceduralWorld PRIVATE include external/stb external/glad/include)

target_link_libraries(ProceduralWorld OpenGL::GL glfw glad glm imgui)

if(OpenMP_CXX_FOUND)
    target_link_libraries(ProceduralWorld OpenMP::OpenMP_CXX)
endif()