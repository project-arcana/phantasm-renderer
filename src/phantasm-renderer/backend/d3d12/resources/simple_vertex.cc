#include "simple_vertex.hh"

#include <fstream>

#include <typed-geometry/tg.hh>

#include <polymesh/Mesh.hh>
#include <polymesh/algorithms.hh>
#include <polymesh/formats.hh>

pr::backend::d3d12::simple_mesh_data pr::backend::d3d12::load_polymesh(const char* path)
{
    pm::Mesh mesh;
    auto mesh_pos = mesh.vertices().make_attribute<tg::pos3>();
    pm::load(path, mesh, mesh_pos);
    pm::normalize(mesh_pos);
    CC_ASSERT(pm::is_triangle_mesh(mesh));

    auto smooth_normals = pm::vertex_normals_by_area(mesh_pos);

    simple_mesh_data res;
    res.vertices.reserve(size_t(mesh.faces().size()) * 3);
    res.indices.reserve(size_t(mesh.faces().size()) * 3);

    for (auto f : mesh.faces())
        for (auto v : f.vertices())
        {
            res.vertices.push_back(simple_vertex{mesh_pos[v], smooth_normals[v]});
            res.indices.push_back(int(res.vertices.size()) - 1);
        }

    return res;
}
