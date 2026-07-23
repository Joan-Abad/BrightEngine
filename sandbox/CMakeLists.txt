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

add_executable(sandbox main.cpp)

target_link_libraries(sandbox
    PRIVATE
        brightengine::brightengine
        glfw
)
