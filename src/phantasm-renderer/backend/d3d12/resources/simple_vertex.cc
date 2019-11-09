#include "simple_vertex.hh"

#include <string>
#include <unordered_map>

#include <typed-geometry/tg-std.hh>

#include <polymesh/Mesh.hh>
#include <polymesh/algorithms.hh>
#include <polymesh/formats.hh>
#include <polymesh/formats/obj.hh>

#include <phantasm-renderer/backend/detail/tiny_obj_loader.hh>

using pr::backend::d3d12::simple_vertex;

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

pr::backend::d3d12::simple_mesh_data pr::backend::d3d12::load_obj_mesh(const char* path, bool flip_uvs)
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
            vertex.position = tg::pos3(-1 * attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                                       attrib.vertices[3 * index.vertex_index + 2]);

            if (index.texcoord_index != -1)
                vertex.texcoord = transform_uv(tg::vec2(attrib.texcoords[2 * index.texcoord_index + 0], attrib.texcoords[2 * index.texcoord_index + 1]));

            if (index.normal_index != -1)
                vertex.normal = tg::vec3(-1 * attrib.normals[3 * index.normal_index + 0], attrib.normals[3 * index.normal_index + 1],
                                         attrib.normals[3 * index.normal_index + 2]);

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

    //    pm::Mesh mesh;
    //    auto reader = pm::obj_reader<float>(path, mesh);
    //    CC_ASSERT(pm::is_triangle_mesh(mesh));
    //    auto const tex_coords = reader.get_tex_coords();
    //    auto const mesh_pos = reader.get_positions().to<tg::pos3>();
    //    auto const smooth_normals = pm::vertex_normals_by_area(mesh_pos);

    //    simple_mesh_data res;
    //    res.vertices.reserve(size_t(mesh.faces().size()) * 3);
    //    res.indices.reserve(size_t(mesh.faces().size()) * 3);

    //    for (auto f : mesh.faces())
    //        for (auto v : f.vertices())
    //        {
    //            auto const first_outgoing_h = v.incoming_halfedges().first();
    //            auto const uv = tex_coords[first_outgoing_h];

    //            res.vertices.push_back(simple_vertex{mesh_pos[v], smooth_normals[v], tg::vec2(uv[0], uv[1])});
    //            res.indices.push_back(int(res.vertices.size()) - 1);
    //        }
}
