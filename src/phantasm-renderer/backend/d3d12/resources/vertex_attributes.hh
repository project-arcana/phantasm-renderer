#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/vector.hh>

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
}
