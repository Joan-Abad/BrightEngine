#pragma once

#include "brightengine/rhi/Handle.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace brightengine::rhi
{
    struct VertexAttribute
    {
        uint32_t location;
        int componentCount;
        size_t offset;
    };

    struct PipelineDesc
    {
        const char* vertexShaderSource;
        const char* fragmentShaderSource;
        std::vector<VertexAttribute> vertexAttributes;
        size_t vertexStride;
    };

    struct PipelineTag {};
    using PipelineHandle = Handle<PipelineTag>;
}
