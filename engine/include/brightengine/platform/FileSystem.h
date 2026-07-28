#pragma once

#include <string>

namespace brightengine
{
    // Directory the running executable lives in, no trailing slash.
    // Throws std::runtime_error if it can't be resolved.
    std::string GetExecutableDirectory();

    // Reads an entire text file into a string. Throws std::runtime_error
    // if the file can't be opened.
    std::string ReadTextFile(const std::string& path);
}
