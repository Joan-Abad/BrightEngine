# brightengine: core engine library target.
#
# This is currently an INTERFACE library — there is no engine source code
# yet, only the CMake scaffolding (target + public include directory) so
# that downstream consumers (sandbox/, future tests) have something stable
# to link against while the RHI/ECS/renderer are designed.
#
# Once real .cpp sources exist, this should become a STATIC (or SHARED)
# library target instead of INTERFACE — see engine/docs/build-conventions.md.

add_library(brightengine INTERFACE)
add_library(brightengine::brightengine ALIAS brightengine)

target_include_directories(brightengine
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(brightengine INTERFACE cxx_std_20)
