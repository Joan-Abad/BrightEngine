#include "brightengine/platform/Window.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace brightengine
{
    namespace
    {
        int ToGLFWKey(KeyCode key)
        {
            switch (key)
            {
                case KeyCode::W:      return GLFW_KEY_W;
                case KeyCode::A:      return GLFW_KEY_A;
                case KeyCode::S:      return GLFW_KEY_S;
                case KeyCode::D:      return GLFW_KEY_D;
                case KeyCode::Escape: return GLFW_KEY_ESCAPE;
            }
            return GLFW_KEY_UNKNOWN;
        }
    }

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

    bool Window::IsKeyPressed(KeyCode key) const
    {
        return glfwGetKey(m_window, ToGLFWKey(key)) == GLFW_PRESS;
    }

    void Window::RequestClose()
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    glm::vec2 Window::GetCursorPosition() const
    {
        double x, y;
        glfwGetCursorPos(m_window, &x, &y);
        return glm::vec2(static_cast<float>(x), static_cast<float>(y));
    }

    void Window::SetCursorCaptured(bool captured)
    {
        glfwSetInputMode(m_window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}
