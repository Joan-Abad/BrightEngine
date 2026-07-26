#pragma once

#include <cstdint>

namespace brightengine::rhi
{
    template <typename Tag>
    struct Handle
    {
        static constexpr uint32_t kInvalidId = 0;

        uint32_t id = kInvalidId;

        bool IsValid() const { return id != kInvalidId; }

        bool operator==(const Handle& other) const { return id == other.id; }
        bool operator!=(const Handle& other) const { return id != other.id; }
    };
}
