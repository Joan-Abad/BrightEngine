#include "OpenGLDevice.h"

#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

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

        bool CompileShader(GLenum stage, const char* source, GLuint& outShader)
        {
            outShader = glCreateShader(stage);
            glShaderSource(outShader, 1, &source, nullptr);
            glCompileShader(outShader);

            int success = 0;
            glGetShaderiv(outShader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                char infoLog[512];
                glGetShaderInfoLog(outShader, sizeof(infoLog), nullptr, infoLog);
                std::fprintf(stderr, "Shader compile error: %s\n", infoLog);
                glDeleteShader(outShader);
                return false;
            }
            return true;
        }
    }

    OpenGLDevice::~OpenGLDevice()
    {
        for (auto& [id, record] : m_buffers)
        {
            glDeleteBuffers(1, &record.glBuffer);
        }
        for (auto& [id, record] : m_pipelines)
        {
            glDeleteProgram(record.program);
            glDeleteVertexArrays(1, &record.vao);
        }
        for (auto& [id, record] : m_textures)
        {
            glDeleteTextures(1, &record.glTexture);
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

    PipelineHandle OpenGLDevice::CreatePipeline(const PipelineDesc& desc)
    {
        GLuint vertexShader = 0;
        if (!CompileShader(GL_VERTEX_SHADER, desc.vertexShaderSource, vertexShader))
        {
            return PipelineHandle{};
        }

        GLuint fragmentShader = 0;
        if (!CompileShader(GL_FRAGMENT_SHADER, desc.fragmentShaderSource, fragmentShader))
        {
            glDeleteShader(vertexShader);
            return PipelineHandle{};
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        int success = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            std::fprintf(stderr, "Shader program link error: %s\n", infoLog);
            glDeleteProgram(program);
            return PipelineHandle{};
        }

        // Bakes the vertex buffer bound to GL_ARRAY_BUFFER right now into this
        // VAO's attribute setup -- call BindVertexBuffer() before CreatePipeline().
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        for (const VertexAttribute& attribute : desc.vertexAttributes)
        {
            glVertexAttribPointer(
                attribute.location,
                attribute.componentCount,
                GL_FLOAT,
                GL_FALSE,
                static_cast<GLsizei>(desc.vertexStride),
                reinterpret_cast<void*>(attribute.offset)
            );
            glEnableVertexAttribArray(attribute.location);
        }

        PipelineHandle handle{ m_nextPipelineId++ };
        m_pipelines[handle.id] = PipelineRecord{ program, vao };

        return handle;
    }

    void OpenGLDevice::DestroyPipeline(PipelineHandle handle)
    {
        auto it = m_pipelines.find(handle.id);
        if (it == m_pipelines.end())
        {
            return;
        }

        glDeleteProgram(it->second.program);
        glDeleteVertexArrays(1, &it->second.vao);
        m_pipelines.erase(it);
    }

    void OpenGLDevice::BindPipeline(PipelineHandle handle)
    {
        const PipelineRecord& record = m_pipelines.at(handle.id);
        glUseProgram(record.program);
        glBindVertexArray(record.vao);
    }

    int OpenGLDevice::GetUniformLocation(PipelineHandle handle, const char* name)
    {
        return glGetUniformLocation(m_pipelines.at(handle.id).program, name);
    }

    void OpenGLDevice::SetUniformMat4(PipelineHandle handle, int location, const glm::mat4& value)
    {
        glUseProgram(m_pipelines.at(handle.id).program);
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

    void OpenGLDevice::SetUniformInt(PipelineHandle handle, int location, int value)
    {
        glUseProgram(m_pipelines.at(handle.id).program);
        glUniform1i(location, value);
    }

    TextureHandle OpenGLDevice::CreateTexture(const TextureDesc& desc)
    {
        GLuint glTexture = 0;
        glGenTextures(1, &glTexture);
        glBindTexture(GL_TEXTURE_2D, glTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, desc.width, desc.height, 0, GL_RGB, GL_UNSIGNED_BYTE, desc.pixelData);

        TextureHandle handle{ m_nextTextureId++ };
        m_textures[handle.id] = TextureRecord{ glTexture };

        return handle;
    }

    void OpenGLDevice::DestroyTexture(TextureHandle handle)
    {
        auto it = m_textures.find(handle.id);
        if (it == m_textures.end())
        {
            return;
        }

        glDeleteTextures(1, &it->second.glTexture);
        m_textures.erase(it);
    }

    void OpenGLDevice::BindTexture(TextureHandle handle, int slot)
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_textures.at(handle.id).glTexture);
    }

    void OpenGLDevice::SetClearColor(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }

    void OpenGLDevice::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void OpenGLDevice::DrawIndexed(uint32_t indexCount)
    {
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    }

    std::unique_ptr<IDevice> CreateDevice()
    {
        return std::make_unique<OpenGLDevice>();
    }
}
