#pragma once

#include "brightengine/rhi/Handle.h"

#include <cstddef>

namespace brightengine::rhi
{
    enum class BufferType
    {
        Vertex,
        Index,
    };

    struct BufferDesc
    {
        BufferType type;
        size_t sizeInBytes;
        const void* initialData = nullptr;
    };

    struct BufferTag {};
    using BufferHandle = Handle<BufferTag>;
}
