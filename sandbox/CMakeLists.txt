# sandbox: minimal app that consumes the engine through its public API.
# Now exercises the RHI directly (brightengine::rhi::CreateDevice()) in
# addition to hand-written OpenGL calls used for exercises ahead of the RHI
# covering that ground itself.
#
# GLFW, GLEW, and GLM are fetched once at the root CMakeLists.txt (shared
# with the `engine` target's GLEW dependency) — see
# engine/docs/build-conventions.md for why they aren't declared here.

add_executable(sandbox main.cpp)

target_link_libraries(sandbox
    PRIVATE
        brightengine::brightengine
        glfw
        libglew_static
        glm::glm
)
