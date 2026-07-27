#pragma once

struct GLFWwindow;

namespace brightengine
{
    // Throws std::runtime_error on failure -- there's no meaningful way to
    // continue without a window/GL context, so construction either fully
    // succeeds or the object never exists.
    class Window
    {
    public:
        Window(int width, int height, const char* title);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        bool ShouldClose() const;
        void PollEvents();
        void SwapBuffers();
        float GetTime() const;

    private:
        GLFWwindow* m_window = nullptr;
    };
}
