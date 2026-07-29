#include "brightengine/assets/Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <stdexcept>
#include <unordered_map>

namespace brightengine
{
    namespace
    {
        // .obj doesn't store an indexed vertex buffer the way a GPU wants --
        // each face corner references a (position, texcoord, normal) index
        // triple independently, so the same position can need a different
        // combined vertex depending on which face it's part of. This key
        // identifies one unique combination so it can be deduplicated back
        // into a real index buffer.
        struct IndexKey
        {
            int vertexIndex;
            int texcoordIndex;

            bool operator==(const IndexKey& other) const
            {
                return vertexIndex == other.vertexIndex && texcoordIndex == other.texcoordIndex;
            }
        };

        struct IndexKeyHash
        {
            size_t operator()(const IndexKey& key) const
            {
                return std::hash<int>()(key.vertexIndex) ^ (std::hash<int>()(key.texcoordIndex) << 1);
            }
        };
    }

    Mesh::Mesh(const std::string& path)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warning;
        std::string error;

        bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, path.c_str());
        if (!ok)
        {
            throw std::runtime_error("Failed to load mesh: " + path + " (" + error + ")");
        }

        std::unordered_map<IndexKey, uint32_t, IndexKeyHash> uniqueVertices;

        for (const tinyobj::shape_t& shape : shapes)
        {
            for (const tinyobj::index_t& index : shape.mesh.indices)
            {
                IndexKey key{ index.vertex_index, index.texcoord_index };

                auto it = uniqueVertices.find(key);
                if (it != uniqueVertices.end())
                {
                    m_indices.push_back(it->second);
                    continue;
                }

                MeshVertex vertex{};
                vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
                vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
                vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];

                if (index.texcoord_index >= 0)
                {
                    vertex.uv[0] = attrib.texcoords[2 * index.texcoord_index + 0];
                    vertex.uv[1] = attrib.texcoords[2 * index.texcoord_index + 1];
                }

                uint32_t newIndex = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
                m_indices.push_back(newIndex);
                uniqueVertices[key] = newIndex;
            }
        }
    }
}
