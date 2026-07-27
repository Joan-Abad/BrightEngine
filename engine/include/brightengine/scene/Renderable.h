#pragma once

#include "brightengine/rhi/Buffer.h"
#include "brightengine/rhi/Pipeline.h"
#include "brightengine/rhi/Texture.h"

#include <cstdint>

namespace brightengine
{
    struct Renderable
    {
        rhi::PipelineHandle pipeline;
        rhi::BufferHandle vertexBuffer;
        rhi::BufferHandle indexBuffer;
        rhi::TextureHandle texture;
        uint32_t indexCount = 0;
    };
}
