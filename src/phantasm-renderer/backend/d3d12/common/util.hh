#pragma once

#include <clean-core/capped_vector.hh>

#include <typed-geometry/tg-lean.hh>

#include <phantasm-renderer/backend/assets/vertex_attrib_info.hh>
#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>

namespace pr::backend::d3d12::util
{
inline void set_viewport(ID3D12GraphicsCommandList* command_list, tg::ivec2 const& size)
{
    auto const viewport = D3D12_VIEWPORT{0.f, 0.f, float(size.x), float(size.y), 0.f, 1.f};
    auto const scissor_rect = D3D12_RECT{0, 0, LONG(size.x), LONG(size.y)};

    command_list->RSSetViewports(1, &viewport);
    command_list->RSSetScissorRects(1, &scissor_rect);
}

inline void populate_barrier_desc(D3D12_RESOURCE_BARRIER& out_barrier, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    out_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    out_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    out_barrier.Transition.pResource = res;
    out_barrier.Transition.StateBefore = before;
    out_barrier.Transition.StateAfter = after;
    out_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
}

[[nodiscard]] cc::capped_vector<D3D12_INPUT_ELEMENT_DESC, 16> get_native_vertex_format(cc::span<vertex_attribute_info const> attrib_info);

void set_object_name(ID3D12Object* object, char const* name, ...);
}
