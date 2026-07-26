#pragma once

#include "brightengine/rhi/Buffer.h"

#include <memory>

namespace brightengine::rhi
{
    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        virtual BufferHandle CreateBuffer(const BufferDesc& desc) = 0;
        virtual void DestroyBuffer(BufferHandle handle) = 0;

        virtual void BindVertexBuffer(BufferHandle handle) = 0;
        virtual void BindIndexBuffer(BufferHandle handle) = 0;
    };

    // Creates the device for the currently active backend (OpenGL only, for now).
    std::unique_ptr<IDevice> CreateDevice();
}
