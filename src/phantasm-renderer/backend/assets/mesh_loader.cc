#include "mesh_loader.hh"

#include <string>
#include <unordered_map>

#include <typed-geometry/tg-std.hh>

#include <phantasm-renderer/backend/detail/tiny_obj_loader.hh>

using pr::backend::assets::simple_vertex;

namespace std
{
template <>
struct hash<simple_vertex>
{
    size_t operator()(simple_vertex const& vertex) const
    {
        return ((hash<tg::pos3>()(vertex.position) ^ (hash<tg::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<tg::vec2>()(vertex.texcoord) << 1);
    }
};
}

pr::backend::assets::simple_mesh_data pr::backend::assets::load_obj_mesh(const char* path, bool flip_uvs, bool flip_xaxis)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warnings, errors;

    auto const ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, path);
    CC_RUNTIME_ASSERT(ret && "Failed to load mesh");

    simple_mesh_data res;
    res.vertices.reserve(attrib.vertices.size() / 3);
    res.indices.reserve(attrib.vertices.size() * 6); // heuristic, possibly also exposed by tinyobjs returns

    std::unordered_map<simple_vertex, int> unique_vertices;
    unique_vertices.reserve(attrib.vertices.size() / 3);

    auto const transform_uv = [&](tg::vec2 uv) {
        if (flip_uvs)
            uv.y = 1.f - uv.y;

        return uv;
    };

    for (auto const& shape : shapes)
    {
        for (auto const& index : shape.mesh.indices)
        {
            simple_vertex vertex = {};
            vertex.position = tg::pos3(attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                                       attrib.vertices[3 * index.vertex_index + 2]);

            if (index.texcoord_index != -1)
                vertex.texcoord = transform_uv(tg::vec2(attrib.texcoords[2 * index.texcoord_index + 0], attrib.texcoords[2 * index.texcoord_index + 1]));

            if (index.normal_index != -1)
                vertex.normal = tg::vec3(attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1],
                                         attrib.normals[3 * index.normal_index + 2]);

            if (flip_xaxis)
            {
                vertex.position.x *= -1;
                vertex.normal.x *= -1;
            }

            if (unique_vertices.count(vertex) == 0)
            {
                unique_vertices[vertex] = int(res.vertices.size());
                res.vertices.push_back(vertex);
            }

            res.indices.push_back(unique_vertices[vertex]);
        }
    }

    res.vertices.shrink_to_fit();
    res.indices.shrink_to_fit();
    return res;
}
