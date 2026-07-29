#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace brightengine
{
    struct MeshVertex
    {
        float position[3];
        float uv[2];
    };

    // Loads a .obj file into an indexed vertex/index list ready for
    // IDevice::CreateBuffer. Throws std::runtime_error on failure. Header
    // stays free of tinyobjloader -- consumers never see the parsing lib.
    class Mesh
    {
    public:
        explicit Mesh(const std::string& path);

        const std::vector<MeshVertex>& GetVertices() const { return m_vertices; }
        const std::vector<uint32_t>& GetIndices() const { return m_indices; }

    private:
        std::vector<MeshVertex> m_vertices;
        std::vector<uint32_t> m_indices;
    };
}
