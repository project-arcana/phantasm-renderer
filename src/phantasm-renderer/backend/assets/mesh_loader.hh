#pragma once

#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

namespace pr::backend::assets {

struct simple_vertex
{
    tg::pos3 position = tg::pos3::zero;
    tg::vec3 normal = tg::vec3(0, 1, 0);
    tg::vec2 texcoord = tg::vec2::zero;

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
};

[[nodiscard]] simple_mesh_data load_obj_mesh(char const* path, bool flip_uvs = true);

}
