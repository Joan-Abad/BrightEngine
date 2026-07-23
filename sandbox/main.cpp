// Minimal sandbox app: build-system smoke test only.
//
// Verifies that GLFW is fetched, compiled, and linked correctly against
// the brightengine target. Creates a window, ticks the event loop a
// bounded number of times, then closes on its own. No OpenGL context
// usage, no engine RHI usage yet.

#include <GLFW/glfw3.h>

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

    constexpr int kMaxIterations = 60;
    int iteration = 0;

    while (!glfwWindowShouldClose(window) && iteration < kMaxIterations)
    {
        glfwPollEvents();
        ++iteration;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
