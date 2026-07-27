#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <brightengine/rhi/Device.h>
#include <cstdio>
#include <memory>

int main()
{
    if (!glfwInit())
    {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "BrightEngine Sandbox", nullptr, nullptr);
    if (!window)
    {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    GLenum Result = glewInit();
    if (Result != GLEW_OK)
    {
        std::fprintf(stderr, "Failed to init GLEW\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

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

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);

    int transformLocation = device->GetUniformLocation(pipeline, "uTransform");
    int viewLocation = device->GetUniformLocation(pipeline, "uView");
    int projectionLocation = device->GetUniformLocation(pipeline, "uProjection");
    int textureLocation = device->GetUniformLocation(pipeline, "uTexture");

    device->SetUniformInt(pipeline, textureLocation, 0);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        640.0f / 480.0f,
        0.1f,
        100.0f
    );

    device->SetUniformMat4(pipeline, viewLocation, view);
    device->SetUniformMat4(pipeline, projectionLocation, projection);

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        device->BindPipeline(pipeline);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        float time = (float)glfwGetTime();
        glm::mat4 transform = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.0f, 1.0f, 0.0f));
        device->SetUniformMat4(pipeline, transformLocation, transform);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    device->DestroyPipeline(pipeline);
    device->DestroyBuffer(vertexBuffer);
    device->DestroyBuffer(indexBuffer);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
