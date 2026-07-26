#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>

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

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

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

    unsigned int vertexShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderHandle, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShaderHandle);

    unsigned int fragmentShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderHandle, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShaderHandle);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShaderHandle, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderHandle, 512, nullptr, infoLog);
        std::fprintf(stderr, "Vertex shader compile error: %s\n", infoLog);
    }

    glGetShaderiv(fragmentShaderHandle, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderHandle, 512, nullptr, infoLog);
        std::fprintf(stderr, "fragment shader compile error: %s\n", infoLog);
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShaderHandle);
    glAttachShader(shaderProgram, fragmentShaderHandle);
    glLinkProgram(shaderProgram);

    int transformLocation = glGetUniformLocation(shaderProgram, "uTransform");
    int viewLocation = glGetUniformLocation(shaderProgram, "uView");
    int projectionLocation = glGetUniformLocation(shaderProgram, "uProjection");
    int textureLocation = glGetUniformLocation(shaderProgram, "uTexture");

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::fprintf(stderr, "Shader program link error: %s\n", infoLog);
    }

    glUseProgram(shaderProgram);
    glUniform1i(textureLocation, 0);

    // Cámara y proyección: no cambian frame a frame en este ejemplo (nada
    // mueve la cámara todavía), así que se calculan una sola vez, fuera
    // del loop.
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f), // posición de la cámara en el mundo
        glm::vec3(0.0f, 0.0f, 0.0f), // punto al que mira
        glm::vec3(0.0f, 1.0f, 0.0f)  // qué dirección es "arriba"
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), // campo de visión vertical
        640.0f / 480.0f,     // aspect ratio (debe coincidir con el tamaño de la ventana)
        0.1f,                // near plane
        100.0f                // far plane
    );

    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        float time = (float)glfwGetTime();
        glm::mat4 transform = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(transformLocation, 1, GL_FALSE, glm::value_ptr(transform));

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    glDeleteShader(vertexShaderHandle);
    glDeleteShader(fragmentShaderHandle);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
