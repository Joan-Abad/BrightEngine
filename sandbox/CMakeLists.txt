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

# sandbox/textures/ needs the exact same treatment as sandbox/shaders/ above
# (see the comment on sandbox_shaders): sandbox/main.cpp resolves
# <brightengine::GetExecutableDirectory()>/textures/checker.png at runtime, so
# sandbox/textures/ has to end up next to the built sandbox executable, and
# has to re-copy whenever a texture file's contents change, not just once.
file(GLOB SANDBOX_TEXTURE_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/textures/*
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/textures.stamp
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/textures
            $<TARGET_FILE_DIR:sandbox>/textures
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/textures.stamp
    DEPENDS ${SANDBOX_TEXTURE_FILES}
    COMMENT "Copying sandbox/textures/ next to the sandbox executable"
)

add_custom_target(sandbox_textures ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/textures.stamp
)

add_dependencies(sandbox sandbox_textures)

# sandbox/models/ needs the exact same treatment as sandbox/shaders/ and
# sandbox/textures/ above (see the comment on sandbox_shaders):
# sandbox/main.cpp resolves
# <brightengine::GetExecutableDirectory()>/models/cube.obj at runtime via
# brightengine::Mesh, so sandbox/models/ has to end up next to the built
# sandbox executable, and has to re-copy whenever a model file's contents
# change, not just once.
file(GLOB SANDBOX_MODEL_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/models/*
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/models.stamp
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/models
            $<TARGET_FILE_DIR:sandbox>/models
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/models.stamp
    DEPENDS ${SANDBOX_MODEL_FILES}
    COMMENT "Copying sandbox/models/ next to the sandbox executable"
)

add_custom_target(sandbox_models ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/models.stamp
)

add_dependencies(sandbox sandbox_models)
