#include "brightengine/platform/Window.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace brightengine
{
    Window::Window(int width, int height, const char* title)
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_window)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(m_window);

        if (glewInit() != GLEW_OK)
        {
            glfwDestroyWindow(m_window);
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLEW");
        }
    }

    Window::~Window()
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_window);
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::SwapBuffers()
    {
        glfwSwapBuffers(m_window);
    }

    float Window::GetTime() const
    {
        // glfwGetTime() is actually a global clock, not per-window -- exposed
        // here as a method purely for convenience/discoverability.
        return static_cast<float>(glfwGetTime());
    }
}
