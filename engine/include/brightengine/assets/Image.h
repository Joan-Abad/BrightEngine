#pragma once

#include <string>

namespace brightengine
{
    // Loads and decodes an image file into raw RGB8 pixels. Throws
    // std::runtime_error if the file can't be found/decoded. Header stays
    // free of stb_image.h -- consumers never see the decoding library.
    class Image
    {
    public:
        explicit Image(const std::string& path);
        ~Image();

        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        const unsigned char* GetPixels() const { return m_pixels; }

    private:
        int m_width = 0;
        int m_height = 0;
        unsigned char* m_pixels = nullptr;
    };
}
