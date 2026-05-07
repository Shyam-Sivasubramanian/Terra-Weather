# ProceduralWorld — Ray Tracer Edition

A CPU ray tracer + procedural terrain + climate/weather simulation, all flowing
through a single shared `WorldData` struct.

## Contents

| Subsystem   | Owner  | Files                                                                      |
|-------------|--------|----------------------------------------------------------------------------|
| Shared      | Both   | `WorldData.h`, `Ray.h`, `HitRecord.h`, `Material.h`, `AABB.h`, `Hittable.h`|
| Core        | Shyam  | `Window` (GL blit only), `Framebuffer`, `Camera`, `Texture2D`              |
| Terrain     | Shyam  | `NoiseGen` (2D + 3D Perlin), `HeightMap`                                   |
| Ray tracer  | Shyam  | `BVH`, `TerrainHittable`, `Materials`, `Scene`, `RayTrace`, `Renderer`     |
| Climate     | Janavi | `HumidityMap`, `WindField`, `WeatherMap`, `Precipitation`, `CloudMap`      |
| Rendering   | Janavi | `Atmosphere`, `VolumetricCloud`, `WeatherVolume`                           |

## Build

Requires CMake ≥ 3.20 and a C++17 compiler. GLFW, GLM, and GLAD are pulled
automatically via `FetchContent`, so the only system-level dependency is OpenGL
3.3 drivers.

```bash
cd Terra-Weather
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/ProceduralWorld
```

On Linux you may additionally need the usual X11/Wayland dev packages that
GLFW expects (`xorg-dev`, `libwayland-dev`, etc.).

## Controls

| Key           | Action                                        |
|---------------|-----------------------------------------------|
| `W A S D`     | move horizontally (hold `Shift` to sprint)    |
| `Q / E`       | descend / ascend                              |
| RMB + drag    | look around                                   |
| `R`           | regenerate with a new random seed             |
| `[` / `]`     | earlier / later time of day                   |
| `F12`         | save `screenshot.png` of current image        |
| `Esc`         | quit                                          |

## Textures

Drop `grass.png`, `rock.png`, `snow.png` into `assets/textures/`. If missing,
the terrain material falls back to plausible solid colours so the build still
runs. An `assets/hdri/sky.hdr` slot is reserved for a future environment-map
fallback but is not required.

## Rendering strategy

1. When the camera moves (or seed changes) the framebuffer is cleared and
   dropped to 1/4 resolution, 1 SPP. A tile-based thread pool (32×32 tiles,
   `hardware_concurrency()` threads) fills a pass.
2. After 4 accumulated samples of preview the framebuffer is promoted to full
   window resolution and keeps refining until the camera moves again.
3. Path termination uses Russian roulette past depth 3.
4. Direct sun sampling is a per-hit shadow ray, attenuated by a single cloud
   transmittance probe — cheap but visibly correct through-cloud dimming.

Expect **1–4 FPS at 1/4 resolution with 1 SPP, `maxDepth = 4`** on an 8-core
machine, as the plan anticipated. A full 1080p 64-SPP render is multi-minute;
use `F12` after letting it refine for 20–30 seconds.

## Deviations from the original plan

- **No ImGui yet.** The plan lists ImGui panels for seed/SPP/thread count.
  Pulling ImGui via `FetchContent` is fiddly (back-end glue choices, version
  mismatches against GLFW). I wired the essentials — seed, time of day,
  screenshot — to the keyboard instead so the build works out of the box. Add
  ImGui as a follow-up if desired.
- **Terrain is chunked, not a single hittable.** Plan §Week 2 describes the
  terrain as one large `TerrainHittable` with internal triangle lookup. I used
  16×16-cell `TerrainChunk` leaves in the top-level BVH instead: chunks give
  the BVH something useful to prune against without pushing single triangles
  (~130 000 of them) into the tree and blowing up build time. Each chunk
  internally linearly tests its own triangles. Intersection math (Möller-
  Trumbore, smooth normals, barycentric UVs) matches the plan.
- **Water is flat within each underwater chunk**, not an infinite reflective
  plane. This keeps shorelines crisp against the terrain mesh.
- **BVH uses median-split** on centroid along the longest-axis of the centroid
  bound, not SAH. Plan §Week 4 lists SAH as an upgrade target.
- **Adaptive sampling is not implemented.** Plan §Week 4 target.
- **Sun/moon transitions are coarse.** The `setSunFromTimeOfDay(0..1)` function
  moves the sun along a great circle and dims at night, but there's no lunar
  disk or star field yet (plan §Week 4).

All data contracts (the fields on `WorldData`, the Week-1 handshake on
`cloudDensity`) are exactly as the plan specifies, so extensions slot in
without touching the shared header.

## File tree

```
Terra-Weather/
├── CMakeLists.txt
├── README.md
├── assets/
│   ├── hdri/           (sky.hdr — optional, not included)
│   └── textures/       (grass.png, rock.png, snow.png — optional)
├── external/
│   ├── stb_image.h
│   └── stb_image_write.h
├── include/
│   ├── AABB.h            Camera.h            Hittable.h          Ray.h
│   ├── Atmosphere.h      Climate.h           Material.h          RayTrace.h
│   ├── BVH.h             CloudMap.h          Materials.h         Renderer.h
│   │                     Framebuffer.h       NoiseGen.h          Scene.h
│   ├── HeightMap.h       HitRecord.h                             TerrainHittable.h
│   ├── Texture2D.h       VolumetricCloud.h   WeatherVolume.h     Window.h
│   └── WorldData.h
└── src/
    ├── main.cpp
    ├── core/             (Window, Framebuffer, Camera, Texture2D)
    ├── terrain/          (NoiseGen, HeightMap)
    ├── raytracer/        (Renderer, Scene, BVH, TerrainHittable, RayTrace,
    │                      Materials, Atmosphere, VolumetricCloud, WeatherVolume)
    └── climate/          (HumidityMap, WindField, WeatherMap,
                           Precipitation, CloudMap)
```
