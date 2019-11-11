#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>


namespace pr::backend::d3d12
{
namespace detail
{
template <class VertT>
struct vertex_visitor
{
    // TODO: Eventually, this will work once the 19.23 bug is fixed
    // static constexpr auto num_attributes = rf::member_count<VertT>;
    // cc::capped_vector<D3D12_INPUT_ELEMENT_DESC, num_attributes> attributes;
    cc::vector<D3D12_INPUT_ELEMENT_DESC> attributes;

    template <class T>
    void operator()(T const& ref, char const* name)
    {
        D3D12_INPUT_ELEMENT_DESC& attr = attributes.emplace_back();
        attr.SemanticName = name;
        attr.SemanticIndex = 0;
        attr.InputSlot = 0;
        attr.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        attr.InstanceDataStepRate = 0;
        attr.Format = util::dxgi_format<T>;

        // This call assumes that the original VertT& in get_vertex_attributes is a dereferenced nullptr
        attr.AlignedByteOffset = UINT(reinterpret_cast<size_t>(&ref));
    }
};
}

template <class VertT>
[[nodiscard]] auto get_vertex_attributes()
{
    detail::vertex_visitor<VertT> visitor;
    VertT* volatile dummyPointer = nullptr; // Volatile to avoid UB-based optimization
    introspect(visitor, *dummyPointer);
    return visitor.attributes;
}

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
