# sandbox: minimal app that consumes the engine through its public API.
# Currently just a build-system smoke test: fetch GLFW, create a window,
# spin the event loop a few times, and close. No RHI usage yet.

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

add_executable(sandbox main.cpp)

target_link_libraries(sandbox
    PRIVATE
        brightengine::brightengine
        glfw
        libglew_static
        glm::glm
)
