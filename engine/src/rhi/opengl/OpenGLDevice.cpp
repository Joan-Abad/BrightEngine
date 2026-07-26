#include "OpenGLDevice.h"

namespace brightengine::rhi
{
    namespace
    {
        GLenum ToGLTarget(BufferType type)
        {
            switch (type)
            {
                case BufferType::Vertex: return GL_ARRAY_BUFFER;
                case BufferType::Index:  return GL_ELEMENT_ARRAY_BUFFER;
            }
            return GL_ARRAY_BUFFER;
        }
    }

    OpenGLDevice::~OpenGLDevice()
    {
        for (auto& [id, record] : m_buffers)
        {
            glDeleteBuffers(1, &record.glBuffer);
        }
    }

    BufferHandle OpenGLDevice::CreateBuffer(const BufferDesc& desc)
    {
        GLuint glBuffer = 0;
        glGenBuffers(1, &glBuffer);

        GLenum target = ToGLTarget(desc.type);
        glBindBuffer(target, glBuffer);
        glBufferData(target, desc.sizeInBytes, desc.initialData, GL_STATIC_DRAW);

        BufferHandle handle{ m_nextBufferId++ };
        m_buffers[handle.id] = BufferRecord{ glBuffer, desc.type };

        return handle;
    }

    void OpenGLDevice::DestroyBuffer(BufferHandle handle)
    {
        auto it = m_buffers.find(handle.id);
        if (it == m_buffers.end())
        {
            return;
        }

        glDeleteBuffers(1, &it->second.glBuffer);
        m_buffers.erase(it);
    }

    void OpenGLDevice::BindVertexBuffer(BufferHandle handle)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_buffers.at(handle.id).glBuffer);
    }

    void OpenGLDevice::BindIndexBuffer(BufferHandle handle)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffers.at(handle.id).glBuffer);
    }

    std::unique_ptr<IDevice> CreateDevice()
    {
        return std::make_unique<OpenGLDevice>();
    }
}
