#pragma once

#include <typed-geometry/types/size.hh>

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/assets/vertex_attrib_info.hh>
#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>

namespace pr::backend::d3d12::util
{
inline void set_viewport(ID3D12GraphicsCommandList* command_list, tg::isize2 size)
{
    auto const viewport = D3D12_VIEWPORT{0.f, 0.f, float(size.width), float(size.height), 0.f, 1.f};
    auto const scissor_rect = D3D12_RECT{0, 0, LONG(size.width), LONG(size.height)};

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

/// create a SRV description based on a shader_view_element
/// the raw resource is only required in case a raytracing AS is described
[[nodiscard]] D3D12_SHADER_RESOURCE_VIEW_DESC create_srv_desc(shader_view_element const& sve, ID3D12Resource* raw_resource);

/// create a UAV description based on a shader_view_element
[[nodiscard]] D3D12_UNORDERED_ACCESS_VIEW_DESC create_uav_desc(shader_view_element const& sve);

/// create a RTV description based on a shader_view_element
[[nodiscard]] D3D12_RENDER_TARGET_VIEW_DESC create_rtv_desc(shader_view_element const& sve);

/// create a DSV description based on a shader_view_element
[[nodiscard]] D3D12_DEPTH_STENCIL_VIEW_DESC create_dsv_desc(shader_view_element const& sve);

/// create a Sampler description based on a sampler config
[[nodiscard]] D3D12_SAMPLER_DESC create_sampler_desc(sampler_config const& config);
}
