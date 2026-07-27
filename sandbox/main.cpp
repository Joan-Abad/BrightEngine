#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <brightengine/platform/Window.h>
#include <brightengine/rhi/Device.h>
#include <brightengine/scene/Camera.h>
#include <brightengine/scene/Scene.h>
#include <cstdio>
#include <memory>
#include <vector>

int main()
{
    try
    {
        brightengine::Window window(640, 480, "BrightEngine Sandbox");

        float vertices[] = {
            // posición            // color                  // UV
            -0.5f, -0.5f, 0.0f,    1.0f, 0.0f, 0.0f,          0.0f, 0.0f, // 0: abajo-izquierda, rojo
             0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,          1.0f, 0.0f, // 1: abajo-derecha, verde
             0.5f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f,          1.0f, 1.0f, // 2: arriba-derecha, azul
            -0.5f,  0.5f, 0.0f,    1.0f, 1.0f, 0.0f,          0.0f, 1.0f  // 3: arriba-izquierda, amarillo
        };

        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        layout (location = 2) in vec2 aTexCoord;

        out vec3 vertexColor;
        out vec2 TexCoord;

        uniform mat4 uTransform;
        uniform mat4 uView;
        uniform mat4 uProjection;

        void main()
        {
            gl_Position = uProjection * uView * uTransform * vec4(aPos, 1.0);
            vertexColor = aColor;
            TexCoord = aTexCoord;
        }
        )";

        const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vertexColor;
        in vec2 TexCoord;
        out vec4 FragColor;

        uniform sampler2D uTexture;

        void main()
        {
            FragColor = texture(uTexture, TexCoord) * vec4(vertexColor, 1.0);
        }
        )";

        std::unique_ptr<brightengine::rhi::IDevice> device = brightengine::rhi::CreateDevice();

        brightengine::rhi::BufferHandle vertexBuffer = device->CreateBuffer({
            brightengine::rhi::BufferType::Vertex,
            sizeof(vertices),
            vertices
        });

        brightengine::rhi::BufferHandle indexBuffer = device->CreateBuffer({
            brightengine::rhi::BufferType::Index,
            sizeof(indices),
            indices
        });

        // CreatePipeline bakes in whichever vertex buffer is bound right now.
        device->BindVertexBuffer(vertexBuffer);

        brightengine::rhi::PipelineHandle pipeline = device->CreatePipeline({
            vertexShaderSource,
            fragmentShaderSource,
            {
                { 0, 3, 0 },
                { 1, 3, 3 * sizeof(float) },
                { 2, 2, 6 * sizeof(float) },
            },
            8 * sizeof(float)
        });
        if (!pipeline.IsValid())
        {
            std::fprintf(stderr, "Failed to create pipeline\n");
            return 1;
        }

        // The pipeline's VAO is bound now (CreatePipeline left it active) -- bind
        // the index buffer here so it gets recorded into that same VAO.
        device->BindIndexBuffer(indexBuffer);

        // Textura generada por código: un tablero de ajedrez 8x8, RGB.
        const int texWidth = 8;
        const int texHeight = 8;
        unsigned char textureData[texWidth * texHeight * 3];
        for (int y = 0; y < texHeight; ++y)
        {
            for (int x = 0; x < texWidth; ++x)
            {
                bool isWhite = (x + y) % 2 == 0;
                unsigned char value = isWhite ? 255 : 0;
                int index = (y * texWidth + x) * 3;
                textureData[index + 0] = value;
                textureData[index + 1] = value;
                textureData[index + 2] = value;
            }
        }

        brightengine::rhi::TextureHandle texture = device->CreateTexture({
            texWidth,
            texHeight,
            textureData
        });

        int transformLocation = device->GetUniformLocation(pipeline, "uTransform");
        int viewLocation = device->GetUniformLocation(pipeline, "uView");
        int projectionLocation = device->GetUniformLocation(pipeline, "uProjection");
        int textureLocation = device->GetUniformLocation(pipeline, "uTexture");

        device->SetUniformInt(pipeline, textureLocation, 0);

        brightengine::Camera camera(
            glm::vec3(0.0f, 0.0f, 3.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            45.0f,
            640.0f / 480.0f,
            0.1f,
            100.0f
        );

        device->SetUniformMat4(pipeline, viewLocation, camera.GetViewMatrix());
        device->SetUniformMat4(pipeline, projectionLocation, camera.GetProjectionMatrix());

        device->SetClearColor(0.1f, 0.2f, 0.3f, 1.0f);

        // Both entities share the same GPU resources (one quad, one shader,
        // one texture) -- only their Transform differs. This is the whole
        // point: geometry/pipeline/texture live once, per-entity state
        // (Transform) is what actually varies.
        uint32_t indexCount = sizeof(indices) / sizeof(indices[0]);

        brightengine::Scene scene;

        brightengine::Entity entityA = scene.CreateEntity();
        scene.GetTransform(entityA).position = glm::vec3(-1.0f, 0.0f, 0.0f);
        scene.GetRenderable(entityA) = { pipeline, vertexBuffer, indexBuffer, texture, indexCount };

        brightengine::Entity entityB = scene.CreateEntity();
        scene.GetTransform(entityB).position = glm::vec3(1.0f, 0.0f, 0.0f);
        scene.GetRenderable(entityB) = { pipeline, vertexBuffer, indexBuffer, texture, indexCount };

        while (!window.ShouldClose())
        {
            window.PollEvents();

            device->Clear();

            float time = window.GetTime();
            scene.GetTransform(entityA).rotationEuler.y = time;
            scene.GetTransform(entityB).rotationEuler.y = -time;

            const std::vector<brightengine::Transform>& transforms = scene.GetTransforms();
            const std::vector<brightengine::Renderable>& renderables = scene.GetRenderables();

            for (size_t i = 0; i < transforms.size(); ++i)
            {
                const brightengine::Renderable& renderable = renderables[i];

                device->BindPipeline(renderable.pipeline);
                device->BindTexture(renderable.texture, 0);
                device->SetUniformMat4(pipeline, transformLocation, transforms[i].GetMatrix());

                device->DrawIndexed(renderable.indexCount);
            }

            window.SwapBuffers();
        }

        device->DestroyPipeline(pipeline);
        device->DestroyTexture(texture);
        device->DestroyBuffer(vertexBuffer);
        device->DestroyBuffer(indexBuffer);
    }
    catch (const std::exception& e)
    {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
