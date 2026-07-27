# brightengine: core engine library target.
#
# STATIC library — the first real engine sources (the OpenGL RHI device
# backend) now exist, so this is no longer an INTERFACE-only scaffold. See
# engine/docs/build-conventions.md for the INTERFACE -> STATIC conversion
# rationale and target-layout rules.

add_library(brightengine STATIC
    src/rhi/opengl/OpenGLDevice.cpp
)
add_library(brightengine::brightengine ALIAS brightengine)

target_include_directories(brightengine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(brightengine PUBLIC cxx_std_20)

# GLEW is used privately by the OpenGL RHI backend (engine/src/rhi/opengl/).
# It must never leak into brightengine's public interface: consumers only
# see brightengine/rhi/Device.h, not GL/glew.h. GLEW itself is fetched once
# at the root CMakeLists.txt (before this add_subdirectory), not here — see
# engine/docs/build-conventions.md.
#
# GLM is PUBLIC: brightengine/rhi/Device.h (a public header) includes
# <glm/glm.hpp> directly (glm::mat4 appears in IDevice's own interface), so
# any consumer of brightengine::brightengine transitively needs GLM's include
# path too. See engine/docs/build-conventions.md.
target_link_libraries(brightengine
    PRIVATE
        libglew_static
    PUBLIC
        glm::glm
)
