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
       `cmake -S . -B build-msvc-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl`
       (or `build-msvc-release` / `-DCMAKE_BUILD_TYPE=Release` — see
       "Build configurations" below).
    3. Build with `cmake --build build-msvc-debug` (or
       `build-msvc-release`).
    Use a build directory name that encodes the toolchain (e.g.
    `build-msvc-debug/`) so it never collides with a MinGW/GCC `build/`
    directory from a different toolchain — never reuse one build
    directory across toolchains.
  - If a future CMake version recognizes the installed VS generator name
    directly, prefer switching to it over the manual Ninja/vcvarsall
    dance — this workaround exists only because of the CMake/VS version
    gap, not because Ninja is preferred over the VS generator on
    principle.
- `CMAKE_EXPORT_COMPILE_COMMANDS ON` is set at the root so editor tooling
  (clangd, etc.) works out of the box.

## Build configurations

Ninja (used for the MSVC toolchain per the workaround above, and also a
common choice on Linux) is a **single-config** generator: unlike the Visual
Studio generator, which bakes both Debug and Release into one `.sln` and
lets you switch configurations at build time, a Ninja build tree only ever
has one `CMAKE_BUILD_TYPE` baked in at configure time. Passing a different
`-DCMAKE_BUILD_TYPE` into an *existing* Ninja build directory does not
reliably give you a clean second configuration — the safe, supported
pattern is one build directory per configuration:

- `build-msvc-debug/` — configured with `-DCMAKE_BUILD_TYPE=Debug`.
- `build-msvc-release/` — configured with `-DCMAKE_BUILD_TYPE=Release`.

Never reuse a single `build-msvc/`-style directory across configurations by
re-running `cmake` with a different `CMAKE_BUILD_TYPE` in place — always
configure a fresh, distinctly-named directory instead. (An earlier session
did use an unqualified `build-msvc/` directory for what was implicitly a
Debug build; it has since been replaced by `build-msvc-debug/` plus the new
`build-msvc-release/`, both configured from a clean directory rather than
by mutating the old one in place, since Ninja build trees are not
guaranteed safe to rename/repurpose after the fact.)

Both directories are configured identically otherwise — same MSVC toolchain
via `vcvarsall.bat x64` and explicit `-DCMAKE_C_COMPILER=cl
-DCMAKE_CXX_COMPILER=cl`, per the Ninja/MSVC workaround above — they only
differ in `CMAKE_BUILD_TYPE`. Verified by a full clean
(`cmake --build <dir> --clean-first`) build of both directories: both
produced a working `sandbox.exe` with zero compiler warnings, and the
`sandbox_shaders` copy step (see "Target layout" below) worked
independently in each, since it's driven by `$<TARGET_FILE_DIR:sandbox>`
inside each build tree rather than a shared/hardcoded path.

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
  - Private backend dependencies (e.g. GLEW for the OpenGL RHI backend, GLFW
    for windowing) are linked `PRIVATE` — they must never leak into
    brightengine's public interface. Consumers include
    `brightengine/rhi/Device.h` and `brightengine/platform/Window.h`, never
    `GL/glew.h` or `GLFW/glfw3.h`.
  - Alongside `rhi/` (RHI backends: public header in
    `include/brightengine/rhi/`, private impl in `src/rhi/<backend>/`), there
    is now a `platform/` subtree for the concrete (non-RHI) windowing
    wrapper: public header `include/brightengine/platform/Window.h` (forward-
    declares the opaque `GLFWwindow` struct, never includes `<GLFW/glfw3.h>`,
    the same "hide the backend library from consumers" pattern the RHI uses
    for GLEW/OpenGL), private impl `src/platform/Window.cpp` (the only place
    in `engine/` that includes `<GLFW/glfw3.h>` directly, plus `<GL/glew.h>`
    for the `glewInit()` call after context creation). `platform/` is
    deliberately not part of the `rhi` namespace/directory: windowing isn't
    multi-backend the way the RHI is, so there's no `IWindow` interface,
    just the one concrete `Window` class.
  - `platform/` also now has a `FileSystem` utility: public header
    `include/brightengine/platform/FileSystem.h` declares
    `GetExecutableDirectory()` and `ReadTextFile()`, private impl
    `src/platform/FileSystem.cpp` branches `#if defined(_WIN32)` (uses
    `<windows.h>`'s `GetModuleFileNameA`, with `WIN32_LEAN_AND_MEAN` defined
    first) vs. `#else` (uses `<unistd.h>`'s `readlink("/proc/self/exe", ...)`
    for Linux). Neither branch needs an extra `target_link_libraries` entry:
    on Windows, `GetModuleFileNameA` resolves against `kernel32.lib`, which
    MSVC's linker already includes by default on every link line (verified
    by inspecting the actual `link.exe` invocation via `cmake --build -v`,
    not just assumed); on Linux, `readlink` is a glibc symbol, always
    available. If a future platform/API needs something MSVC doesn't
    default-link (e.g. `Shlwapi.lib`, `Dbghelp.lib`), add it explicitly to
    `target_link_libraries(brightengine PRIVATE ...)` and document it here.
- `sandbox/` builds an executable named `sandbox` that links against
  `brightengine::brightengine` and `glm::glm` — nothing else engine-related,
  and (as of `brightengine::Window` absorbing all direct GLFW/GLEW calls)
  no windowing/GL-loader library directly either. It may only reach into the
  engine's public RHI/platform API, never into `engine/src`. GLM is still
  linked directly because `sandbox/main.cpp` uses `<glm/glm.hpp>` itself
  (transformation matrices) for hand-written OpenGL exercises not yet folded
  into the RHI; that's expected to shrink over time as more of that logic
  moves behind `brightengine::rhi`.
  - `sandbox` used to also link `glfw` and `libglew_static` directly, back
    when `main.cpp` called GLFW/GLEW functions itself. Once that code moved
    into `brightengine::Window` (which links both `PRIVATE`), those two
    direct links in `sandbox/CMakeLists.txt` became redundant and were
    removed — verified by actually deleting them and doing a full clean
    reconfigure + rebuild (not just assumed). This works because
    `brightengine` is a `STATIC` library: CMake still adds a `PRIVATE`
    static-library dependency's own link items (`glfw.lib`, `glewd.lib`)
    to the *final linker command line* of anything consuming that static
    library, even though it doesn't propagate their include paths/compile
    definitions. `PRIVATE` only means "don't propagate as a usage
    requirement (headers, `-D` flags) to consumers" — it does not mean
    "don't propagate at final-link time for a static library archive",
    since the archive itself has no runtime existence to link against
    independently; the symbols still have to come from somewhere at the
    final `sandbox.exe` link. If `brightengine` ever becomes `SHARED`
    instead, this stops being true (a shared library resolves its own
    private dependencies internally) and any direct consumer need would
    have to be re-evaluated then.
- Public headers live under `engine/include/brightengine/...` and are the
  only headers `sandbox/` (or any future consumer) may include.
- `sandbox/main.cpp` loads its GLSL shaders from real files on disk (no
  longer embedded C++ string literals) via
  `brightengine::GetExecutableDirectory() + "/shaders/" + <name>`, so
  `sandbox/shaders/` must exist next to the built `sandbox`/`sandbox.exe`
  binary at runtime. `sandbox/CMakeLists.txt` handles this with a
  GLOB-dependent stamp file rather than a plain `POST_BUILD` command
  attached directly to the `sandbox` target: a `POST_BUILD` custom command
  only reruns when that target's own link step reruns, so if a shader
  file's *contents* change but nothing in `main.cpp` forces a relink, a
  naive `POST_BUILD copy_directory` would silently go stale. Instead:
  `file(GLOB SANDBOX_SHADER_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*)` feeds `DEPENDS` on an
  `add_custom_command(OUTPUT .../shaders.stamp COMMAND ${CMAKE_COMMAND} -E
  copy_directory ... $<TARGET_FILE_DIR:sandbox>/shaders COMMAND
  ${CMAKE_COMMAND} -E touch .../shaders.stamp ...)`, wrapped in an
  `add_custom_target(sandbox_shaders ALL DEPENDS .../shaders.stamp)` that
  `sandbox` depends on via `add_dependencies`. `CONFIGURE_DEPENDS` makes
  CMake re-glob at build time so adding/removing a shader file is picked
  up without a manual reconfigure, and the build tool (Ninja) only reruns
  the copy when a globbed file's mtime is newer than the stamp. Verified by
  a full clean configure+build with the MSVC/Ninja toolchain below and
  confirming `shaders/basic.vert`/`basic.frag` actually land in
  `<build-dir>/sandbox/shaders/`, next to `sandbox.exe`
  (`$<TARGET_FILE_DIR:sandbox>` rather than a hardcoded path, so this
  keeps working for a multi-config generator too, even though the project
  currently only builds single-config Ninja). Any future asset directory
  (textures, models, audio, ...) added under `sandbox/` needs the same
  treatment — a plain one-time copy or a `POST_BUILD` copy tied to the
  executable's own relink is not sufficient.
- `sandbox/textures/` (currently just `checker.png`, loaded via
  `brightengine::Image` + `brightengine::GetExecutableDirectory() +
  "/textures/checker.png"` in `main.cpp`) uses the exact same
  GLOB-dependent-stamp-file + custom-target pattern as `sandbox/shaders/`
  above, duplicated as its own `SANDBOX_TEXTURE_FILES` /
  `textures.stamp` / `sandbox_textures` custom target (rather than
  generalizing both into one parameterized helper — two copies of a
  ~15-line pattern was judged not worth a `function()` yet; revisit if a
  third asset directory shows up). Verified the same way: a full clean
  reconfigure+build in both `build-msvc-debug/` and `build-msvc-release/`
  lands `sandbox/textures/checker.png` at
  `$<TARGET_FILE_DIR:sandbox>/textures/checker.png` in each.
- `sandbox/models/` (currently just `cube.obj`, loaded via
  `brightengine::Mesh` + `brightengine::GetExecutableDirectory() +
  "/models/cube.obj"` in `main.cpp`) uses the exact same
  GLOB-dependent-stamp-file + custom-target pattern as `sandbox/shaders/`
  and `sandbox/textures/` above, duplicated as its own
  `SANDBOX_MODEL_FILES` / `models.stamp` / `sandbox_models` custom target.
  Verified the same way: a full clean reconfigure+build in both
  `build-msvc-debug/` and `build-msvc-release/` lands
  `sandbox/models/cube.obj` at `$<TARGET_FILE_DIR:sandbox>/models/cube.obj`
  in each.

## Dependencies

All third-party dependencies are pulled via `FetchContent` (`include(
FetchContent)` + `FetchContent_Declare` + `FetchContent_MakeAvailable`),
pinned to a stable tag (never a floating branch). No vendored binaries, no
git submodules, unless a specific dependency requires it and that
requirement is documented here when it happens.

All current dependencies are declared once in the **root**
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
instead. GLM doesn't strictly need this on its own (only `engine`'s public
RHI header and `sandbox/main.cpp` use it directly), but it's kept alongside
GLFW/GLEW at the root too, both for consistency and because more than one
target already needs it. GLFW and GLEW are now both linked by `engine`
itself (`platform/Window.cpp` and, for GLEW, `rhi/opengl/OpenGLDevice.cpp`
too) as well as being fetched once here, which is exactly the
declare-once-at-the-root scenario this paragraph describes.

Current dependencies:

- **GLFW** (`https://github.com/glfw/glfw.git`, tag `3.4`) — window/input
  creation. zlib/libpng license, actively maintained, cross-platform
  (Windows/Linux/macOS), no overlapping dependency already in the project.
  Declared in the root `CMakeLists.txt` with `GLFW_BUILD_EXAMPLES`,
  `GLFW_BUILD_TESTS`, `GLFW_BUILD_DOCS`, and `GLFW_INSTALL` forced `OFF` to
  avoid building/installing anything beyond the library itself. Linked
  `PRIVATE` into `engine` (as `glfw`), used by `engine/src/platform/
  Window.cpp` — GLFW calls no longer happen in `sandbox/main.cpp` at all
  (they're wrapped behind `brightengine::Window`), so `sandbox` does not
  link `glfw` directly anymore; it gets the symbols transitively through
  `brightengine::brightengine` at final-link time (see "Target layout"
  above for why that works for a `STATIC` library).
- **GLEW** (`https://github.com/Perlmint/glew-cmake.git`, tag
  `glew-cmake-2.3.1`) — OpenGL function loader. Used by two consumers now,
  both inside `engine`: `engine/src/rhi/opengl/OpenGLDevice.cpp` (the OpenGL
  RHI backend) and `engine/src/platform/Window.cpp` (the `glewInit()` call
  after GL context creation), both linked `PRIVATE` — never exposed through
  `brightengine`'s public headers. `sandbox/main.cpp` no longer calls GLEW
  directly either (same reasoning as GLFW above), so it doesn't link
  `libglew_static` directly anymore. Modified BSD / MIT / Khronos license
  (same license terms as upstream GLEW), no overlapping dependency already
  in the project.
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
  not provide a namespaced/aliased target name) into `engine` only
  (`PRIVATE`) — not into `sandbox` anymore, see above.

- **stb** (`https://github.com/nothings/stb.git`, commit
  `31c1ad37456438565541f4919958214b6e762fb4`) — only `stb_image.h` is used,
  for decoding image files (`engine/src/assets/Image.cpp`, backing
  `sandbox/textures/checker.png`). Public domain (unlicense) / MIT dual
  license (pick either), a widely used and still actively maintained
  collection of single-header libraries, and doesn't duplicate anything
  else already in the project (GLFW/GLEW/GLM are windowing/GL-loading/math,
  not image decoding). Unlike GLFW/GLEW/GLM, this repo has **no
  `CMakeLists.txt` of its own** — it's a loose collection of single-header
  libraries with nothing to configure or build. `FetchContent_Declare` +
  `FetchContent_MakeAvailable(stb)` therefore just populates
  `${stb_SOURCE_DIR}` without an implicit `add_subdirectory()` (there's no
  `CMakeLists.txt` there for it to descend into) and exposes no CMake
  target at all. `engine/CMakeLists.txt` manually adds `${stb_SOURCE_DIR}`
  to `target_include_directories(brightengine PRIVATE ...)` so
  `src/assets/Image.cpp`'s `#include <stb_image.h>` resolves; `PRIVATE`
  because `brightengine/assets/Image.h` (the public header) does not
  include `stb_image.h` itself (same "hide the backend library" pattern as
  GLEW/GLFW above), so it must never leak into `brightengine`'s public
  interface.
  stb does not use version tags in any normal sense (`git ls-remote --tags`
  against the repo returns nothing — confirmed when this dependency was
  added), so unlike GLFW/GLEW/GLM's tag pins, this is pinned to a specific
  commit SHA instead, which still satisfies "never a floating branch": it
  was the tip of `master` at the time this dependency was added. `stb_image.h`
  itself does the usual single-header-library dance: the header only
  declares functions, and exactly one translation unit across the whole
  link must `#define STB_IMAGE_IMPLEMENTATION` before including it to get
  the actual definitions — that's `engine/src/assets/Image.cpp`, and only
  that file; no other `.cpp` in the project may do the same `#define` or
  the link will fail with duplicate symbols.

- **tinyobjloader** (`https://github.com/tinyobjloader/tinyobjloader.git`, tag
  `v2.0.0rc13`) — `.obj` model loading, used by `engine/src/assets/Mesh.cpp`
  (backing `sandbox/models/cube.obj`). MIT-licensed, actively maintained
  (the `release` branch has commits well past this tag), does not duplicate
  stb: stb_image decodes raster image formats (PNG/JPG/...), tinyobjloader
  parses a completely different, text-based 3D mesh format (Wavefront
  `.obj`) — no functional overlap.
  Unlike stb, this repo *does* ship its own root `CMakeLists.txt`
  (`add_library(tinyobjloader ...)` compiling `tiny_obj_loader.cc`, which
  itself does the identical single-header-style
  `#define TINYOBJLOADER_IMPLEMENTATION` + `#include` dance internally) — but
  it is deliberately **not** consumed that way here, unlike GLFW/GLEW/GLM.
  An earlier version of this project declared it with
  `FetchContent_Declare` + `FetchContent_MakeAvailable(tinyobjloader)`
  (the same pattern as GLFW/GLEW/GLM) and linked the resulting
  `tinyobjloader` target `PRIVATE` into `engine`. That "worked" — both
  Debug and Release linked with no duplicate-symbol errors — but only by
  accident: `engine/src/assets/Mesh.cpp` already does its own
  `#define TINYOBJLOADER_IMPLEMENTATION` + `#include <tiny_obj_loader.h>`
  (the same pattern used for `stb_image.h`), so `Mesh.cpp.obj` already
  defines every `tinyobj::` symbol the final link needs. A static-library
  archive member (`tiny_obj_loader.cc.obj` inside `tinyobjloader.lib`) is
  only pulled into a link if some *other* object file has an unresolved
  reference to a symbol only it provides — since nothing did, the linker
  silently never extracted it, and the project built a second, redundant
  compiled copy of the exact same implementation code for no benefit, with
  no guarantee that stayed true (e.g. if a future translation unit ever
  referenced `tinyobj::` symbols without its own
  `TINYOBJLOADER_IMPLEMENTATION` define, or if link order/dedup behavior
  differed on another toolchain).
  This has been fixed: tinyobjloader is now treated exactly like stb — a
  single-header-style library where `Mesh.cpp` is the one and only
  translation unit providing the implementation, so its own
  `CMakeLists.txt` must never be processed (no `add_subdirectory`, no
  second compiled `tinyobjloader.lib`). The root `CMakeLists.txt` still
  does `FetchContent_Declare(tinyobjloader ...)` (same tag/URL as before),
  but instead of `FetchContent_MakeAvailable(tinyobjloader)` it does:

  ```cmake
  FetchContent_GetProperties(tinyobjloader)
  if(NOT tinyobjloader_POPULATED)
      FetchContent_Populate(tinyobjloader)
  endif()
  ```

  `FetchContent_Populate()` fetches the source into
  `${tinyobjloader_SOURCE_DIR}` without an implicit `add_subdirectory()`
  step (there's nothing there for it to configure), and the
  `FetchContent_GetProperties()`/`tinyobjloader_POPULATED` guard makes this
  safe to re-run across configure passes (mirrors the classic pre-3.14
  "populate only" `FetchContent` pattern). Checked against this project's
  CMake version (3.27.0): calling `FetchContent_Populate(tinyobjloader)`
  this way emits **no deprecation warning** — the deprecation notice CMake
  eventually added for `FetchContent_Populate()` (discouraging it in favor
  of `FetchContent_MakeAvailable()` + `EXCLUDE_FROM_ALL`) was introduced in
  a later CMake release than what's installed here; verified by actually
  reconfiguring both `build-msvc-debug` and `build-msvc-release` from
  scratch and checking the configure log for any CMake `Warning`/`Deprecat`
  output — there was none. If this project's CMake is ever upgraded past
  the version that introduces that warning, revisit this and switch to
  whatever mechanism CMake recommends at that point instead of carrying a
  now-discouraged call forward unquestioned.
  `engine/CMakeLists.txt` now adds `${tinyobjloader_SOURCE_DIR}` to
  `target_include_directories(brightengine PRIVATE ...)`, right alongside
  `${stb_SOURCE_DIR}` (the header `tiny_obj_loader.h` lives at that repo's
  root, same as `stb_image.h`) — and there is no
  `target_link_libraries(brightengine PRIVATE tinyobjloader)` line anymore;
  that target no longer exists in the build tree at all. `PRIVATE` because
  `brightengine/assets/Mesh.h` (the public header) does not include
  `<tiny_obj_loader.h>`, same "hide the backend library" pattern as
  GLEW/GLFW/stb above.
  Verified by a full clean reconfigure + build of both `build-msvc-debug`
  and `build-msvc-release` (see "Build configurations" below): both
  produce a working `sandbox.exe` with zero compiler/linker warnings, the
  actual `link.exe` command line for `sandbox.exe` in both configurations
  contains no `tinyobjloader.lib`/`tinyobjloader.dir` reference at all (only
  `brightengine.lib`, `glm.lib`, `glew.lib`/`glewd.lib`, `glfw3.lib`,
  `opengl32.lib`, and the default Windows system libs), and
  `_deps/tinyobjloader-build/` in each build tree is empty (confirming its
  `CMakeLists.txt` was never processed) while
  `_deps/tinyobjloader-subbuild/` still exists (that's `FetchContent`'s own
  internal populate-step scaffolding, not the library's build).

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
