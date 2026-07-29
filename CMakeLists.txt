cmake_minimum_required(VERSION 3.20)

project(BrightEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json for tooling (clangd, etc.).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Third-party dependencies (GLFW, GLEW, GLM) are declared once here, at the
# root, rather than in engine/ or sandbox/ individually. Both `engine` (the
# OpenGL RHI backend) and `sandbox` need GLEW; FetchContent_Declare +
# FetchContent_MakeAvailable is not idempotent-safe to call from two
# separately add_subdirectory'd CMakeLists.txt for the same dependency name
# -- MakeAvailable internally add_subdirectory()s the fetched source once
# the content is populated, so declaring/making available "glew" from both
# engine/CMakeLists.txt and sandbox/CMakeLists.txt would try to add the
# glew-cmake source directory twice and fail with duplicate target errors
# (libglew_static defined twice). Declaring all three dependencies here,
# before either add_subdirectory(), gives both targets a single shared copy
# to link against. See engine/docs/build-conventions.md for the full
# rationale and per-dependency justification.
include(FetchContent)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glfw)

# GLEW (OpenGL function loader). Use the Perlmint/glew-cmake fork, not the
# upstream nigels-com/glew repo: upstream's CMakeLists.txt lives under
# build/cmake instead of the repo root, which is awkward to consume with
# FetchContent. The fork provides a root CMakeLists.txt built for this
# exact use case. We only need the static library, so the shared variant
# and the glewinfo/visualinfo helper executables are disabled.
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ONLY_LIBS ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG        glew-cmake-2.3.1
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glew)

# GLM (OpenGL Mathematics). Header-only, so unlike GLFW/GLEW there's no
# library to build and no example/test/tool options to force off — it just
# exposes the `glm::glm` INTERFACE target once fetched.
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glm)

# stb_image (single-header image loading library, part of the nothings/stb
# collection). Unlike GLFW/GLEW/GLM, this repo has no CMakeLists.txt of its
# own -- FetchContent_MakeAvailable() just populates the source without an
# add_subdirectory() step (there's nothing to configure/build: it's headers
# only, and stb doesn't even ship an INTERFACE target for us to link
# against). engine/CMakeLists.txt manually adds ${stb_SOURCE_DIR} as a
# PRIVATE include directory on the `brightengine` target instead. See
# engine/docs/build-conventions.md for why the commit below (rather than a
# version tag) is pinned.
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(stb)

# tinyobjloader (.obj model loading, engine/src/assets/Mesh.cpp). This repo
# ships its own root CMakeLists.txt (add_library(tinyobjloader ...) compiling
# tiny_obj_loader.cc, which itself does the identical single-header
# #define TINYOBJLOADER_IMPLEMENTATION + #include dance internally) -- but
# Mesh.cpp already does that same #define+#include itself, so letting
# FetchContent_MakeAvailable() add_subdirectory() this and link the
# resulting "tinyobjloader" target would build a second, redundant copy of
# the exact same implementation. Unlike GLFW/GLEW/GLM (real dependencies we
# want built+linked as libraries), tinyobjloader is used here exactly like
# stb: a single-header-style library where engine/src/assets/Mesh.cpp is the
# one and only translation unit providing the implementation. So its
# CMakeLists.txt is deliberately never processed -- FetchContent_Populate()
# (called directly, after FetchContent_GetProperties() guards against
# re-populating on a second configure) fetches the source without an
# add_subdirectory() step, exposing only ${tinyobjloader_SOURCE_DIR} for
# engine/CMakeLists.txt to add as an include directory, same as
# ${stb_SOURCE_DIR}. See engine/docs/build-conventions.md for the full
# rationale (this replaced an earlier FetchContent_MakeAvailable-based
# approach that built a redundant tinyobjloader.lib) and for why the tag
# below (rather than a newer non-tagged commit) is pinned.
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        v2.0.0rc13
    GIT_SHALLOW    TRUE
)

FetchContent_GetProperties(tinyobjloader)
if(NOT tinyobjloader_POPULATED)
    FetchContent_Populate(tinyobjloader)
endif()

add_subdirectory(engine)
add_subdirectory(sandbox)
