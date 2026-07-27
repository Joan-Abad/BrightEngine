#pragma once

#include "brightengine/rhi/Buffer.h"
#include "brightengine/rhi/Pipeline.h"

#include <glm/glm.hpp>
#include <memory>

namespace brightengine::rhi
{
    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        //START BUFFERS
        virtual BufferHandle CreateBuffer(const BufferDesc& desc) = 0;
        virtual void DestroyBuffer(BufferHandle handle) = 0;

        virtual void BindVertexBuffer(BufferHandle handle) = 0;
        virtual void BindIndexBuffer(BufferHandle handle) = 0;

        //END BUFFERS

        //START PIPELINE
        virtual PipelineHandle CreatePipeline(const PipelineDesc& desc) = 0;
        virtual void DestroyPipeline(PipelineHandle handle) = 0;
        virtual void BindPipeline(PipelineHandle handle) = 0;

        virtual int GetUniformLocation(PipelineHandle handle, const char* name) = 0;
        virtual void SetUniformMat4(PipelineHandle handle, int location, const glm::mat4& value) = 0;
        virtual void SetUniformInt(PipelineHandle handle, int location, int value) = 0;
        //END PIPELINE  
    };

    // Creates the device for the currently active backend (OpenGL only, for now).
    std::unique_ptr<IDevice> CreateDevice();
}
