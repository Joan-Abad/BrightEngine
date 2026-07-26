#pragma once

#include "brightengine/rhi/Device.h"

#include <GL/glew.h>
#include <unordered_map>

namespace brightengine::rhi
{
    class OpenGLDevice final : public IDevice
    {
    public:
        ~OpenGLDevice() override;

        BufferHandle CreateBuffer(const BufferDesc& desc) override;
        void DestroyBuffer(BufferHandle handle) override;

        void BindVertexBuffer(BufferHandle handle) override;
        void BindIndexBuffer(BufferHandle handle) override;

    private:
        struct BufferRecord
        {
            GLuint glBuffer = 0;
            BufferType type = BufferType::Vertex;
        };

        std::unordered_map<uint32_t, BufferRecord> m_buffers;
        uint32_t m_nextBufferId = 1;
    };
}
