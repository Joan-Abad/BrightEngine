#include "brightengine/platform/FileSystem.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace brightengine
{
    std::string GetExecutableDirectory()
    {
        std::string fullPath;

#if defined(_WIN32)
        char path[MAX_PATH];
        DWORD length = GetModuleFileNameA(nullptr, path, MAX_PATH);
        if (length == 0)
        {
            throw std::runtime_error("Failed to resolve executable path");
        }
        fullPath.assign(path, length);
#else
        char path[PATH_MAX];
        ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (length == -1)
        {
            throw std::runtime_error("Failed to resolve executable path");
        }
        fullPath.assign(path, length);
#endif

        size_t lastSlash = fullPath.find_last_of("/\\");
        return fullPath.substr(0, lastSlash);
    }

    std::string ReadTextFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
}
