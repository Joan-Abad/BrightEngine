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

# sandbox/main.cpp resolves shader paths at runtime as
# <brightengine::GetExecutableDirectory()>/shaders/<name>, so sandbox/shaders/
# has to end up next to the built sandbox executable. This is implemented as
# a GLOB-dependent stamp file + custom target (rather than a plain POST_BUILD
# copy_directory attached directly to the `sandbox` target) so the copy
# re-runs whenever a shader file's *contents* change, even on a build where
# main.cpp/sandbox itself doesn't need to relink (a POST_BUILD command on
# `sandbox` only fires when that target's own link step reruns). See
# engine/docs/build-conventions.md for the full rationale. Any future asset
# directory (textures, models, etc.) added under sandbox/ will need the same
# treatment.
file(GLOB SANDBOX_SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders.stamp
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:sandbox>/shaders
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/shaders.stamp
    DEPENDS ${SANDBOX_SHADER_FILES}
    COMMENT "Copying sandbox/shaders/ next to the sandbox executable"
)

add_custom_target(sandbox_shaders ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders.stamp
)

add_dependencies(sandbox sandbox_shaders)
