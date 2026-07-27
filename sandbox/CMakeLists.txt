# sandbox: minimal app that consumes the engine through its public API.
# Exercises the RHI (brightengine::rhi::CreateDevice()) and windowing
# (brightengine::Window) entirely through brightengine's public headers now
# — main.cpp no longer calls GLFW/GLEW directly, so sandbox only needs to
# link brightengine::brightengine (which privately links glfw/libglew_static
# itself) plus glm::glm for its own direct GLM use (transformation matrices
# in main.cpp). See engine/docs/build-conventions.md for why GLFW/GLEW are
# NOT listed here even though sandbox's executable ultimately needs their
# symbols at final link time.

add_executable(sandbox main.cpp)

target_link_libraries(sandbox
    PRIVATE
        brightengine::brightengine
        glm::glm
)
