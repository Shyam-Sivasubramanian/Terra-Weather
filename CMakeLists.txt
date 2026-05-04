cmake_minimum_required(VERSION 3.20)
project(ProceduralWorld CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Warning / optimization flags ---
if(MSVC)
  add_compile_options(/W3)
else()
  add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# --- FetchContent for dependencies ---
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# GLFW -----------------------------------------------------------------------
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS     OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS    OFF CACHE INTERNAL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_INSTALL        OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(glfw)

# GLM ------------------------------------------------------------------------
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# GLAD (OpenGL 3.3 core loader) ----------------------------------------------
# We vendor a tiny loader via Dav1dde/glad's pre-generated output. Instead of
# pulling the glad generator (which requires Python), we fetch a pre-generated
# copy from Dav1dde/glad-binaries. If that proves flaky, a user can drop their
# own glad files in external/glad and comment this block out.
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v0.1.36
)
set(GLAD_PROFILE "core"     CACHE STRING "" FORCE)
set(GLAD_API     "gl=3.3"   CACHE STRING "" FORCE)
set(GLAD_GENERATOR "c"      CACHE STRING "" FORCE)
set(GLAD_SPEC    "gl"       CACHE STRING "" FORCE)
set(GLAD_NO_LOADER OFF      CACHE BOOL "" FORCE)
set(GLAD_INSTALL  OFF       CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glad)

# --- OpenGL / threads ---
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

# --- OpenMP (optional fallback for parallel loops) ---
find_package(OpenMP)

# --- Executable -------------------------------------------------------------
add_executable(ProceduralWorld
    src/main.cpp
    src/core/Window.cpp
    src/core/Framebuffer.cpp
    src/core/Camera.cpp
    src/core/Texture2D.cpp

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
    src/climate/CloudMap.cpp
)

target_include_directories(ProceduralWorld PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external      # stb lives here
)

target_link_libraries(ProceduralWorld PRIVATE
    glfw
    glad
    glm::glm
    OpenGL::GL
    Threads::Threads
)

if(OpenMP_CXX_FOUND)
  target_link_libraries(ProceduralWorld PRIVATE OpenMP::OpenMP_CXX)
endif()

# macOS needs these frameworks explicitly for GLFW's Cocoa back-end.
if(APPLE)
  target_link_libraries(ProceduralWorld PRIVATE
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework OpenGL"
  )
endif()

# Copy assets folder next to the built binary so relative paths resolve.
add_custom_command(TARGET ProceduralWorld POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:ProceduralWorld>/assets
)
