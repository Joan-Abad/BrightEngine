#pragma once

#include "brightengine/rhi/Device.h"
#include "brightengine/scene/Renderable.h"

#include <cstddef>
#include <vector>

namespace brightengine
{
    struct RenderableDesc
    {
        const void* vertexData;
        size_t vertexDataSize;
        const void* indexData;
        size_t indexDataSize;
        uint32_t indexCount;
        const char* vertexShaderSource;
        const char* fragmentShaderSource;
        std::vector<rhi::VertexAttribute> vertexAttributes;
        size_t vertexStride;
        rhi::TextureHandle texture;
    };

    // Bundles buffer + pipeline creation into one call, including the VAO
    // ordering rule (vertex buffer bound before CreatePipeline, index buffer
    // after). Check renderable.pipeline.IsValid() afterwards -- same as
    // calling CreatePipeline directly, this doesn't throw on shader errors.
    inline Renderable CreateRenderable(rhi::IDevice& device, const RenderableDesc& desc)
    {
        rhi::BufferHandle vertexBuffer = device.CreateBuffer({
            rhi::BufferType::Vertex,
            desc.vertexDataSize,
            desc.vertexData
        });

        rhi::BufferHandle indexBuffer = device.CreateBuffer({
            rhi::BufferType::Index,
            desc.indexDataSize,
            desc.indexData
        });

        device.BindVertexBuffer(vertexBuffer);

        rhi::PipelineHandle pipeline = device.CreatePipeline({
            desc.vertexShaderSource,
            desc.fragmentShaderSource,
            desc.vertexAttributes,
            desc.vertexStride
        });

        device.BindIndexBuffer(indexBuffer);

        return Renderable{ pipeline, vertexBuffer, indexBuffer, desc.texture, desc.indexCount };
    }
}
