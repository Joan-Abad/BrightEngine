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

        PipelineHandle CreatePipeline(const PipelineDesc& desc) override;
        void DestroyPipeline(PipelineHandle handle) override;
        void BindPipeline(PipelineHandle handle) override;

        int GetUniformLocation(PipelineHandle handle, const char* name) override;
        void SetUniformMat4(PipelineHandle handle, int location, const glm::mat4& value) override;
        void SetUniformInt(PipelineHandle handle, int location, int value) override;

        TextureHandle CreateTexture(const TextureDesc& desc) override;
        void DestroyTexture(TextureHandle handle) override;
        void BindTexture(TextureHandle handle, int slot) override;

    private:
        struct BufferRecord
        {
            GLuint glBuffer = 0;
            BufferType type = BufferType::Vertex;
        };

        struct PipelineRecord
        {
            GLuint program = 0;
            GLuint vao = 0;
        };

        struct TextureRecord
        {
            GLuint glTexture = 0;
        };

        std::unordered_map<uint32_t, BufferRecord> m_buffers;
        uint32_t m_nextBufferId = 1;

        std::unordered_map<uint32_t, PipelineRecord> m_pipelines;
        uint32_t m_nextPipelineId = 1;

        std::unordered_map<uint32_t, TextureRecord> m_textures;
        uint32_t m_nextTextureId = 1;
    };
}
