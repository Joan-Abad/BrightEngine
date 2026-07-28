#include "brightengine/assets/Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <stdexcept>

namespace brightengine
{
    Image::Image(const std::string& path)
    {
        // Force 3 channels (RGB) regardless of the source file's format, to
        // match what IDevice::CreateTexture currently assumes (GL_RGB).
        m_pixels = stbi_load(path.c_str(), &m_width, &m_height, nullptr, 3);
        if (!m_pixels)
        {
            throw std::runtime_error("Failed to load image: " + path);
        }
    }

    Image::~Image()
    {
        stbi_image_free(m_pixels);
    }
}
