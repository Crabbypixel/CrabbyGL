# CrabbyGL - C++ based Voxel Engine

> A Minecraft-inspired voxel engine built from scratch in **C++ and OpenGL 3.3**

![C++](https://img.shields.io/badge/C++-17-blue?logo=cplusplus)
![OpenGL](https://img.shields.io/badge/OpenGL-3.3-green?logo=opengl)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)
![Status](https://img.shields.io/badge/Status-Active%20Development-orange)

---

## Introduction

CrabbyGL is a fully custom voxel world engine — no game engine, no Unity, no Godot.  
Every system is written from scratch: chunk streaming, mesh building, physics, lighting, disk saves, and the rendering pipeline.

The goal: understand how Minecraft actually works by building it piece by piece.

---

## Demo

![Demo](https://github.com/Crabbypixel/CrabbyGL/blob/main/assets/CrabbyGL.png?raw=true)

---

## Features

### Infinite World Streaming
The world is split into **16 × 256 × 16 block chunks**. As the player moves, new chunks are generated in the background and old ones are unloaded — just like Minecraft.

- View distance: **8 chunks** (~128 blocks)
- Unload distance: **12 chunks** (hysteresis prevents thrashing)
- Chunks are stored in a hash map keyed by their `(chunkX, chunkZ)` coordinate

### Multithreaded Architecture
Four separate thread pools run concurrently:

```
Main Thread
 ├── Chunk Streaming   → decides what to load/unload
 ├── Renderer          → uploads meshes to GPU, draws the world
 └── Player / Input    → physics, raycasting, camera

Load Workers  (×4)    → generate terrain, load from disk
Mesh Workers  (×4)    → build vertex data for each chunk  
Save Worker   (×1)    → write modified chunks to disk (non-blocking)
```

Thread safety is handled via `shared_mutex` per chunk — multiple mesh workers can read a chunk simultaneously, but block placement locks it exclusively. A **staging swap pattern** is used so workers never hold locks while the main thread renders — the critical section is O(1).

### Rendering Pipeline

Every frame goes through this pipeline:

```
1. Bind custom Framebuffer (off-screen render target)
2. Frustum Cull    → discard chunks outside the camera's view pyramid
3. Chunk Draw      → for each visible chunk, issue one draw call
   ├── Vertex Shader  → transform positions, pass UVs + AO to fragment
   └── Fragment Shader
       ├── Sample texture atlas (one texture, all blocks)
       ├── Grass overlay blend (tinted green layer on top of dirt)
       ├── Directional lighting (top/bottom/side face brightness)
       ├── Ambient Occlusion (darkens corners and crevices)
       └── Distance fog (blends to sky color at chunk edge)
4. Crosshair + Debug overlay
5. Blit Framebuffer → default screen
```

### Mesh Building (Exposed-Face Culling)
Only visible block faces become geometry. Hidden faces (buried underground or touching another solid block) are never added to the mesh. This is computed per chunk and per face across chunk borders.

- **Cross-chunk seam culling** — if a face touches a neighbor chunk, that neighbor's block is checked too
- **Diagonal neighbor access** — AO sampling reaches all 8 surrounding chunks (4 axial + 4 diagonal)
- Mesh jobs capture raw chunk pointers upfront, fully decoupled from live world state

### Ambient Occlusion
Per-vertex AO using the Minecraft method — each corner samples 3 neighbors (side1, side2, corner) in the face plane:

```
ao = (side1 && side2) ? 0 : 3 - side1 - side2 - corner
```

Quads are **flipped** when the AO gradient would otherwise cause visual artifacts (the Minecraft anisotropy fix). Applied in the fragment shader as a smooth darkening curve.

### Player Physics
Custom AABB collision system — no physics library used.

- Axis-separated resolution: **X → Y → Z** (prevents corner-locking)
- Epsilon-contracted AABB sweeps to avoid sampling adjacent block faces at exact boundaries
- Gravity, jumping, fly mode toggle
- **DDA raycasting** for block targeting (digital differential analyzer — exact integer grid traversal)

### Disk Persistence
Chunks are saved to `saves/cx_cz.bin` in binary format. Modified chunks are queued to the save worker on unload — the main thread never waits on disk IO.

- `dirty` flag: chunk needs mesh rebuild
- `modified` flag: chunk has unsaved block changes

### Block Registry
Block types are defined as data, not code. Each block has:

```cpp
struct BlockDef {
    const char* name;
    int faces[6];      // texture tile index per face (±Y, ±X, ±Z)
    int overlay;       // secondary texture layer (grass sides)
    glm::vec3 tint;    // color multiplier (green for grass)
    uint16_t flags;    // OPAQUE | SOLID | TRANSPARENT | EMISSIVE
    float hardness;
};
```

No `switch` statements in the mesh builder — all block behavior reads from `GetDef(type)`.

---

## Architecture Overview

```
┌─────────────────────────────────────────────┐
│                  Main Thread                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  │
│  │  Player  │  │  World   │  │ Renderer │  │
│  │  Physics │  │Streaming │  │ SyncGPU  │  │
│  └──────────┘  └────┬─────┘  └──────────┘  │
└───────────────────── │ ──────────────────────┘
                       │ job queues + staging maps
          ┌────────────┼────────────┐
          ▼            ▼            ▼
   ┌─────────────┐ ┌──────────┐ ┌──────────┐
   │ Load Workers│ │   Mesh   │ │  Save    │
   │  ×4 threads │ │ Workers  │ │  Worker  │
   │ (terrain gen│ │ ×4 thread│ │ ×1 thread│
   │  + disk IO) │ │(CPU mesh │ │(disk IO) │
   └─────────────┘ │ build)   │ └──────────┘
                   └──────────┘
```

---

## Block Types

| Block | ID | Notes |
|---|---|---|
| Air | 0 | Transparent, skipped in mesh |
| Dirt | 1 | |
| Grass | 2 | Tinted overlay on side faces |
| Stone | 3 | |
| Bedrock | 4 | |
| Brick | 5 | |
| Tree Log | 6 | Different top/side textures |
| Tree Leaves | 7 | Transparent |

---

## Roadmap

### Done
- [x] Infinite chunk streaming with worker threads
- [x] Disk save / load (binary, non-blocking)
- [x] Exposed-face culling with cross-chunk seams
- [x] Gribb/Hartmann frustum culling
- [x] Texture atlas with per-face UV mapping
- [x] Ambient Occlusion (vertex AO, quad flip)
- [x] Grass overlay blending in shader
- [x] Distance fog
- [x] AABB physics (axis-separated)
- [x] DDA raycasting (block targeting)
- [x] Block placement and breaking
- [x] Fly mode + dual camera (1st/3rd person)
- [x] Block registry (data-driven block definitions)
- [x] Framebuffer post-process pass
- [x] ImGui debug overlay

### In Progress
- [ ] Block expansion — Sand, Gravel, Water, Lava, Coal Ore, Iron Ore
- [ ] 20 TPS world tick system (accumulator pattern)
- [ ] Sand/Gravel fall (queue-based cellular automaton)
- [ ] 2D hotbar UI (orthographic projection, atlas icons)

### Planned
- [ ] Player model + skin (6-part box rig, walk animation)
- [ ] BFS sky-light propagation (cave darkening, Minecraft-accurate)
- [ ] Beta 1.7.3 faithful terrain generation (ported Java → C++)
- [ ] Biomes (temperature/rain noise affecting surface blocks)
- [ ] Worm caves with branching
- [ ] Greedy meshing
- [ ] Emscripten web build (playable in browser)

---

## Technical Stack

| | |
|---|---|
| Language | C++17 |
| Graphics API | OpenGL 3.3 Core |
| Math | GLM |
| Windowing | GLFW |
| Image Loading | stb_image |
| Noise | stb_perlin |
| UI | Dear ImGui |
| Build | Visual Studio 2022 (Community) |

---

## Building

1. Clone the repo
2. Open the `.sln` file in **Visual Studio Community**
3. Set configuration to `Release` and platform to `x64`
4. Hit **Build → Build Solution** (`Ctrl+Shift+B`)
5. Run from the project root so relative asset paths (`assets/`) resolve correctly

> **Requirements:** Visual Studio 2022 (Community or higher), GPU with OpenGL 3.3 support

---

## Controls

| Key | Action |
|---|---|
| `WASD` | Move |
| `Space` | Jump / Fly up |
| `Left Shift` | Fly down |
| `Left Ctrl` | Sprint |
| `Tab` | Toggle fly mode |
| `Left Click` | Break block |
| `Right Click` | Place block |
| `T` | Toggle Ambient Occlusion |
| `Y` | Toggle linear/quadratic AO |
| `G` | Toggle chunk borders |

---

## Project Structure

```
CrabbyGL/
├── src/
│   ├── Main.cpp              # Window, game loop, input
│   ├── world/
│   │   ├── World.cpp         # Chunk streaming, threading, save/load
│   │   ├── Chunk.cpp         # Block storage, mutex
│   │   ├── ChunkMeshBuilder  # Exposed-face culling, AO, mesh gen
│   │   ├── BlockRegistry.cpp # Block definitions
│   │   └── WorldGen.cpp      # Beta 1.7.3 terrain (pending connection)
│   ├── player/
│   │   └── Player.cpp        # AABB physics, DDA raycast
│   └── rendering/
│       └── ChunkMesh.cpp     # GPU buffer management
├── assets/
│   ├── shaders/
│   │   └── ChunkMesh.glsl    # Vertex + fragment (AO, fog, lighting)
│   └── textures/
│       └── textures.png      # 256×16 texture atlas
└── includes/                 # Headers
```

---

## Why build this?

I have always been fascinated with Minecraft's rendering system and world management, they look simple on the surface but hide a lot of depth — chunk threading, seam culling, ambient occlusion, lock-free staging, DDA raycasting. Building it from scratch forces you to actually understand each system instead of calling an API that hides it.

This project is a ground-up implementation of every system, written in C++ with no engine or abstraction layer between the code and the GPU.

Built as a hobby project.

---

*Inspired by Minecraft (Mojang). Not affiliated with or endorsed by Mojang or Microsoft.*
