#pragma once

#include "brightengine/rhi/Handle.h"

namespace brightengine::rhi
{
    struct TextureDesc
    {
        int width;
        int height;
        const void* pixelData;
    };

    struct TextureTag {};
    using TextureHandle = Handle<TextureTag>;
}
