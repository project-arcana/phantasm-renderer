#pragma once

#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

namespace pr::backend::assets
{
struct simple_vertex
{
    tg::pos3 position = tg::pos3(0, 0, 0);
    tg::vec3 normal = tg::vec3(0, 1, 0);
    tg::vec2 texcoord = tg::vec2(0, 0);

    constexpr bool operator==(simple_vertex const& rhs) const noexcept
    {
        return position == rhs.position && normal == rhs.normal && texcoord == rhs.texcoord;
    }
};

template <class I>
constexpr void introspect(I&& i, simple_vertex& v)
{
    i(v.position, "position");
    i(v.normal, "normal");
    i(v.texcoord, "texcoord");
}

struct simple_mesh_data
{
    cc::vector<int> indices;
    cc::vector<simple_vertex> vertices;

    unsigned get_vertex_size_bytes() const { return unsigned(sizeof(vertices[0]) * vertices.size()); }
    unsigned get_index_size_bytes() const { return unsigned(sizeof(indices[0]) * indices.size()); }
};

[[nodiscard]] simple_mesh_data load_obj_mesh(char const* path, bool flip_uvs = true, bool flip_xaxis = true);

}
