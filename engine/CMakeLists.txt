# brightengine: core engine library target.
#
# STATIC library — the first real engine sources (the OpenGL RHI device
# backend) now exist, so this is no longer an INTERFACE-only scaffold. See
# engine/docs/build-conventions.md for the INTERFACE -> STATIC conversion
# rationale and target-layout rules.

add_library(brightengine STATIC
    src/rhi/opengl/OpenGLDevice.cpp
    src/platform/Window.cpp
    src/platform/FileSystem.cpp
    src/assets/Image.cpp
)
add_library(brightengine::brightengine ALIAS brightengine)

target_include_directories(brightengine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        # stb (stb_image.h) has no CMakeLists.txt/target of its own -- see
        # the root CMakeLists.txt's FetchContent_Declare(stb) comment and
        # engine/docs/build-conventions.md. Only src/assets/Image.cpp
        # includes <stb_image.h>, and brightengine/assets/Image.h (the
        # public header) does not, so this stays PRIVATE, same reasoning as
        # GLEW below.
        ${stb_SOURCE_DIR}
)

target_compile_features(brightengine PUBLIC cxx_std_20)

# GLEW is used privately by the OpenGL RHI backend (engine/src/rhi/opengl/)
# and now also by src/platform/Window.cpp (glewInit() after context creation).
# It must never leak into brightengine's public interface: consumers only
# see brightengine/rhi/Device.h and brightengine/platform/Window.h, not
# GL/glew.h. GLEW itself is fetched once at the root CMakeLists.txt (before
# this add_subdirectory), not here — see engine/docs/build-conventions.md.
#
# GLFW is used privately by src/platform/Window.cpp (glfwInit/glfwCreateWindow/
# etc.). Window.h (the public header) only forward-declares the opaque
# GLFWwindow struct, never includes <GLFW/glfw3.h>, so GLFW stays out of
# brightengine's public interface too — linked PRIVATE, same reasoning as
# GLEW. See engine/docs/build-conventions.md.
#
# GLM is PUBLIC: brightengine/rhi/Device.h (a public header) includes
# <glm/glm.hpp> directly (glm::mat4 appears in IDevice's own interface), so
# any consumer of brightengine::brightengine transitively needs GLM's include
# path too. See engine/docs/build-conventions.md.
target_link_libraries(brightengine
    PRIVATE
        libglew_static
        glfw
    PUBLIC
        glm::glm
)
