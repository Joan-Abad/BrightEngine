#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <brightengine/assets/Image.h>
#include <brightengine/platform/FileSystem.h>
#include <brightengine/platform/KeyCode.h>
#include <brightengine/platform/Window.h>
#include <brightengine/rhi/Device.h>
#include <brightengine/scene/Camera.h>
#include <brightengine/scene/Scene.h>
#include <cstdio>
#include <memory>
#include <string>
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

        std::string shaderDir = brightengine::GetExecutableDirectory() + "/shaders/";
        std::string vertexShaderSource = brightengine::ReadTextFile(shaderDir + "basic.vert");
        std::string fragmentShaderSource = brightengine::ReadTextFile(shaderDir + "basic.frag");

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
            vertexShaderSource.c_str(),
            fragmentShaderSource.c_str(),
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

        brightengine::Image checkerImage(brightengine::GetExecutableDirectory() + "/textures/checker.png");

        brightengine::rhi::TextureHandle texture = device->CreateTexture({
            checkerImage.GetWidth(),
            checkerImage.GetHeight(),
            checkerImage.GetPixels()
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

        float lastFrameTime = window.GetTime();
        const float cameraSpeed = 2.0f; // units per second

        while (!window.ShouldClose())
        {
            window.PollEvents();

            float time = window.GetTime();
            float deltaTime = time - lastFrameTime;
            lastFrameTime = time;

            if (window.IsKeyPressed(brightengine::KeyCode::Escape))
            {
                window.RequestClose();
            }
            if (window.IsKeyPressed(brightengine::KeyCode::W))
            {
                camera.MoveForward(cameraSpeed * deltaTime);
            }
            if (window.IsKeyPressed(brightengine::KeyCode::S))
            {
                camera.MoveForward(-cameraSpeed * deltaTime);
            }
            if (window.IsKeyPressed(brightengine::KeyCode::A))
            {
                camera.MoveRight(-cameraSpeed * deltaTime);
            }
            if (window.IsKeyPressed(brightengine::KeyCode::D))
            {
                camera.MoveRight(cameraSpeed * deltaTime);
            }

            device->Clear();
            device->SetUniformMat4(pipeline, viewLocation, camera.GetViewMatrix());

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
