#include "simple_vertex.hh"
#ifdef PR_BACKEND_D3D12

#include <fstream>

#include <typed-geometry/tg.hh>

#include <polymesh/Mesh.hh>
#include <polymesh/formats.hh>
#include <polymesh/algorithms.hh>

pr::backend::d3d12::simple_mesh_data pr::backend::d3d12::load_polymesh(const char *path)
{
    pm::Mesh mesh;
    auto mesh_pos = mesh.vertices().make_attribute<tg::pos3>();
    pm::load(path, mesh, mesh_pos);
    pm::normalize(mesh_pos);
    CC_ASSERT(pm::is_triangle_mesh(mesh));

    simple_mesh_data res;
    res.vertices.reserve(mesh.faces().size() * 3);
    res.indices.reserve(mesh.faces().size() * 3);

    for (auto f : mesh.faces())
    {
        auto const face_verts = f.vertices().to_array<3>(mesh_pos);
        auto const triangle = tg::triangle3(face_verts[0], face_verts[1], face_verts[2]);
        auto const normal = tg::normal(triangle);

        for (auto const& v : face_verts)
        {
            res.vertices.push_back(simple_vertex{v, normal});
            res.indices.push_back(int(res.vertices.size()) - 1);
        }
    }

    return res;
}


#endif
