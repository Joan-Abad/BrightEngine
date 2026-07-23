# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project state

BrightEngine is a 2D/3D game engine in C++20, early scaffold stage. There is no real engine code yet:

- `engine/` is currently an **INTERFACE library** (public include directory only, no `.cpp` sources). It must become a `STATIC`/`SHARED` library target once real sources are added — don't keep it INTERFACE past that point.
- `sandbox/` is a build-system smoke test app: it opens a GLFW window and closes itself after a bounded loop. No rendering yet.

Planned architecture: an RHI (Render Hardware Interface) abstraction designed from day one so the engine can add Vulkan/D3D12/Metal backends later without touching consumer code. OpenGL is the first backend to be implemented behind it.

Target platforms: Windows and Linux.

The original Visual Studio "Hello World" template (`BrightEngine.slnx` / `BrightEngine/BrightEngine.vcxproj`) has been retired and deleted — CMake is the real build system now (see below), not MSBuild.

## Build

CMake (>= 3.20), C++20 (`CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`).

- Root `CMakeLists.txt` — defines the `BrightEngine` project, adds `engine/` and `sandbox/`.
- `engine/CMakeLists.txt` — the `brightengine` target (aliased `brightengine::brightengine`).
- `sandbox/CMakeLists.txt` — a minimal executable consuming `brightengine`'s public API plus GLFW.
- Third-party dependencies are pulled via CMake `FetchContent`, pinned to stable tags (never a floating branch) — no vendored binaries or submodules unless a dependency specifically requires it.

**Compiler:**

- **Windows: MSVC only — never MinGW/GCC**, even if a MinGW toolchain is on `PATH`. Needed for good Direct3D12 support later and VS debugger integration.
- **Linux:** GCC or Clang.

Full toolchain details (which VS install to use vs. avoid, the current CMake-generator-vs-VS2026-version workaround, dependency justifications, target-layout rules, warning-flag policy) live in `engine/docs/build-conventions.md` — read it before changing build configuration, and update it (not this file) when a build convention changes.

There is no test project, no lint configuration, and no CI setup yet.

## Agents

Custom Claude Code subagents live in `.claude/agents/`:

- `build-engineer` — build system, CMake, dependencies, CI. Not RHI/gameplay/architecture docs.

More will be added as RHI/ECS/rendering work starts (e.g. `rhi-renderer`). Each agent's `.md` should stay focused on stable conventions and point to a living doc (like `engine/docs/build-conventions.md`) for details that evolve, rather than growing the agent definition itself.
