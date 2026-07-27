# Build conventions

This document records durable build-system decisions for BrightEngine so the
root `CLAUDE.md`/task instructions don't have to grow indefinitely. Update
this file when a new convention is adopted; don't duplicate its contents
elsewhere.

## Toolchain

- CMake >= 3.20, C++20 (`CMAKE_CXX_STANDARD 20`, `CMAKE_CXX_STANDARD_REQUIRED
  ON`, `CMAKE_CXX_EXTENSIONS OFF`).
- On Linux, `Ninja` or `Unix Makefiles` with GCC/Clang.
- On Windows, **MSVC is the required compiler — never MinGW/GCC**, even if
  a MinGW toolchain happens to be on `PATH`. The first sandbox smoke test
  was accidentally built with MinGW because that's what was on `PATH` at
  the time; that was never an intentional decision and must not be
  repeated.
  - Use the Visual Studio install that actually ships the C++ build tools
    (in this environment: `F:\Microsoft Visual Studio 2026`, Community,
    MSVC toolset ~14.50). Do **not** use a Visual Studio install that is
    managed by another tool (e.g. a Unity Hub-managed install under
    `F:\Unity`) — it's not meant to be used as a general-purpose
    toolchain and may be changed/removed by that tool without warning.
  - As of CMake 3.27, the CMake Visual Studio generator does not yet
    recognize brand-new VS versions by name (e.g. no
    `"Visual Studio 18 2026"` generator exists). When the VS-named
    generator isn't available, do not fall back to MinGW — instead drive
    MSVC's `cl.exe` directly with the **`Ninja`** generator:
    1. Enter the MSVC x64 developer environment via that VS install's
       `VC\Auxiliary\Build\vcvarsall.bat x64` (this also puts VS's
       bundled `ninja.exe`, under
       `Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\`, on `PATH`).
    2. Configure with
       `cmake -S . -B build-msvc -G Ninja -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl`.
    3. Build with `cmake --build build-msvc`.
    Use a build directory name that encodes the toolchain (e.g.
    `build-msvc/`) so it never collides with a MinGW/GCC `build/`
    directory from a different toolchain — never reuse one build
    directory across toolchains.
  - If a future CMake version recognizes the installed VS generator name
    directly, prefer switching to it over the manual Ninja/vcvarsall
    dance — this workaround exists only because of the CMake/VS version
    gap, not because Ninja is preferred over the VS generator on
    principle.
- `CMAKE_EXPORT_COMPILE_COMMANDS ON` is set at the root so editor tooling
  (clangd, etc.) works out of the box.

## Target layout

- `engine/` builds a single target named `brightengine` (aliased as
  `brightengine::brightengine`). It was originally an `INTERFACE` library
  (no sources, only the public include directory), but as of the first RHI
  OpenGL backend sources (`engine/src/rhi/opengl/OpenGLDevice.cpp`) it is a
  `STATIC` library. Do not revert it to `INTERFACE`; if dynamic linking is
  ever needed (hot-reload/tooling), switch to `SHARED` instead.
  - Public include dir (`engine/include/`) is `PUBLIC`.
  - `engine/src/` is added as a `PRIVATE` include dir for consistency, even
    though the current OpenGL backend source includes its sibling header
    (`OpenGLDevice.h`) with a relative `#include "OpenGLDevice.h"`, which
    CMake/the compiler resolves via the including file's own directory
    without needing this on the include path. Keep it declared anyway so
    future `engine/src` files that need to reach across subdirectories
    (e.g. `#include "rhi/opengl/OpenGLDevice.h"` from elsewhere in `src/`)
    don't silently depend on relative-path luck.
  - `target_compile_features(... cxx_std_20)` is `PUBLIC` (not `INTERFACE`):
    the library itself now compiles C++20 code, not just its consumers.
  - Private backend dependencies (e.g. GLEW for the OpenGL RHI backend) are
    linked `PRIVATE` — they must never leak into brightengine's public
    interface. Consumers include `brightengine/rhi/Device.h`, never
    `GL/glew.h`.
- `sandbox/` builds an executable named `sandbox` that links against
  `brightengine::brightengine` and nothing else engine-related — it may only
  reach into the engine's public RHI API, never into `engine/src`. (It also
  links GLFW/GLEW/GLM directly today for the hand-written OpenGL exercises
  in `main.cpp` that haven't been folded into the RHI yet; that's expected
  to shrink over time as more of that logic moves behind `brightengine::rhi`.)
- Public headers live under `engine/include/brightengine/...` and are the
  only headers `sandbox/` (or any future consumer) may include.

## Dependencies

All third-party dependencies are pulled via `FetchContent` (`include(
FetchContent)` + `FetchContent_Declare` + `FetchContent_MakeAvailable`),
pinned to a stable tag (never a floating branch). No vendored binaries, no
git submodules, unless a specific dependency requires it and that
requirement is documented here when it happens.

All three current dependencies are declared once in the **root**
`CMakeLists.txt`, before `add_subdirectory(engine)`/`add_subdirectory(sandbox)`,
rather than in `engine/CMakeLists.txt` or `sandbox/CMakeLists.txt`
individually. This is not just tidiness: `FetchContent_MakeAvailable(<name>)`
internally does an `add_subdirectory()` on the fetched source the first time
it populates `<name>`. Once `engine` needed GLEW too (for the OpenGL RHI
backend), declaring+making-available "glew" separately from both
`engine/CMakeLists.txt` and `sandbox/CMakeLists.txt` would have populated
the same source twice and tried to `add_subdirectory()` it twice, producing
a "target `libglew_static` already defined" error the second time. Declaring
each dependency exactly once at the root, ahead of both `add_subdirectory`
calls, means both targets link against the same already-populated targets
instead. GLFW and GLM don't strictly need this today (only `sandbox` uses
them directly), but they're kept alongside GLEW at the root too, both for
consistency and because the RHI is expected to need GLM for its own math
utilities before long, which would hit the same problem GLEW did.

Current dependencies:

- **GLFW** (`https://github.com/glfw/glfw.git`, tag `3.4`) — window/input
  creation. zlib/libpng license, actively maintained, cross-platform
  (Windows/Linux/macOS), no overlapping dependency already in the project.
  Declared in the root `CMakeLists.txt` with `GLFW_BUILD_EXAMPLES`,
  `GLFW_BUILD_TESTS`, `GLFW_BUILD_DOCS`, and `GLFW_INSTALL` forced `OFF` to
  avoid building/installing anything beyond the library itself. Linked into
  `sandbox` only (as `glfw`) — `engine` does not need window/input handling.
- **GLEW** (`https://github.com/Perlmint/glew-cmake.git`, tag
  `glew-cmake-2.3.1`) — OpenGL function loader. Used by two consumers now:
  `engine/src/rhi/opengl/OpenGLDevice.cpp` (the OpenGL RHI backend, linked
  `PRIVATE` — never exposed through `brightengine`'s public headers) and
  `sandbox/main.cpp` directly (for the hand-written OpenGL exercises not yet
  folded into the RHI). Modified BSD / MIT / Khronos license (same license
  terms as upstream GLEW), no overlapping dependency already in the project.
  Uses the `Perlmint/glew-cmake` fork rather than the upstream
  `nigels-com/glew` repo: upstream's `CMakeLists.txt` lives under
  `build/cmake` instead of the repo root, which needs `SOURCE_SUBDIR` and
  is awkward with `FetchContent`. The fork provides a root-level
  `CMakeLists.txt` built for exactly this use case and is a common,
  actively-maintained choice in the community for this purpose. Declared in
  the root `CMakeLists.txt` with `glew-cmake_BUILD_SHARED` forced `OFF`
  (only the static variant is needed), `glew-cmake_BUILD_STATIC` forced
  `ON`, and `ONLY_LIBS` forced `ON` to skip building the `glewinfo`/
  `visualinfo` helper executables. Linked as `libglew_static` (the fork does
  not provide a namespaced/aliased target name) into both `engine`
  (`PRIVATE`) and `sandbox` (`PRIVATE`).

- **GLM** (`https://github.com/g-truc/glm.git`, tag `1.0.1`) — vector/matrix
  math (transformation matrices, etc.), originally added for the
  hand-written OpenGL exercises in `sandbox/main.cpp`. MIT-licensed (dual
  MIT/"Happy Bunny" as of the 1.0 series), actively maintained, no
  overlapping dependency already in the project. Header-only, so unlike
  GLFW/GLEW there's nothing to build and no example/test/tool options to
  force off with `CACHE BOOL FORCE` — `FetchContent_MakeAvailable(glm)`
  alone makes the `glm::glm` `INTERFACE` target available. Declared in the
  root `CMakeLists.txt`. Linked `PUBLIC` into `engine` (as `glm::glm`): the
  RHI's public header `brightengine/rhi/Device.h` includes `<glm/glm.hpp>`
  directly (`glm::mat4` appears in `IDevice::SetUniformMat4`'s signature),
  so GLM is part of `brightengine`'s own public interface, not just an
  implementation detail — `PUBLIC` (not `PRIVATE`) makes that propagate
  automatically to any consumer of `brightengine::brightengine`, instead of
  relying on every consumer coincidentally linking GLM itself. Also linked
  into `sandbox` directly (as `glm::glm`) for the hand-written OpenGL
  exercises in `main.cpp` that haven't been folded into the RHI yet; that
  link is redundant with the transitive one from `brightengine` for
  RHI-related uses, but still needed for `main.cpp`'s own direct GLM use.

## Warnings / flags

No project-wide warning flags have been set yet (no real engine sources
exist to compile). When engine sources are added, this section must be
updated with the agreed-upon warning level (e.g. `-Wall -Wextra` on
GCC/Clang, `/W4` on MSVC) and whether warnings are treated as errors.
