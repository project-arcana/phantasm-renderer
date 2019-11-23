#pragma once

#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include "d3d12_sanitized.hh"

namespace pr::backend::d3d12::util
{
[[nodiscard]] inline constexpr D3D12_RESOURCE_STATES to_native(resource_state state)
{
    using rs = resource_state;
    switch (state)
    {
    case rs::undefined:
    case rs::unknown:
        return D3D12_RESOURCE_STATE_COMMON;

    case rs::vertex_buffer:
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case rs::index_buffer:
        return D3D12_RESOURCE_STATE_INDEX_BUFFER;

    case rs::constant_buffer:
        return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    case rs::shader_resource:
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case rs::unordered_access:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    case rs::render_target:
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case rs::depth_read:
        return D3D12_RESOURCE_STATE_DEPTH_READ;
    case rs::depth_write:
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;

    case rs::indirect_argument:
        return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

    case rs::copy_src:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case rs::copy_dest:
        return D3D12_RESOURCE_STATE_COPY_DEST;

    case rs::present:
        return D3D12_RESOURCE_STATE_PRESENT;

    case rs::raytrace_accel_struct:
        return D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    }


    return D3D12_RESOURCE_STATE_COMMON;
}

[[nodiscard]] inline constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE to_native(pr::primitive_topology topology)
{
    switch (topology)
    {
    case pr::primitive_topology::triangles:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case pr::primitive_topology::lines:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case pr::primitive_topology::points:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case pr::primitive_topology::patches:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
    }
}

[[nodiscard]] inline constexpr D3D12_PRIMITIVE_TOPOLOGY to_native_topology(pr::primitive_topology topology)
{
    switch (topology)
    {
    case pr::primitive_topology::triangles:
        return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case pr::primitive_topology::lines:
        return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case pr::primitive_topology::points:
        return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case pr::primitive_topology::patches:
        return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST; // TODO
    }
}

[[nodiscard]] inline constexpr D3D12_COMPARISON_FUNC to_native(pr::depth_function depth_func)
{
    switch (depth_func)
    {
    case pr::depth_function::none:
        return D3D12_COMPARISON_FUNC_LESS; // sane defaults
    case pr::depth_function::less:
        return D3D12_COMPARISON_FUNC_LESS;
    case pr::depth_function::less_equal:
        return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case pr::depth_function::greater:
        return D3D12_COMPARISON_FUNC_GREATER;
    case pr::depth_function::greater_equal:
        return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case pr::depth_function::equal:
        return D3D12_COMPARISON_FUNC_EQUAL;
    case pr::depth_function::not_equal:
        return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case pr::depth_function::always:
        return D3D12_COMPARISON_FUNC_ALWAYS;
    case pr::depth_function::never:
        return D3D12_COMPARISON_FUNC_NEVER;
    }
}

[[nodiscard]] inline constexpr D3D12_CULL_MODE to_native(pr::cull_mode cull_mode)
{
    switch (cull_mode)
    {
    case pr::cull_mode::none:
        return D3D12_CULL_MODE_NONE;
    case pr::cull_mode::back:
        return D3D12_CULL_MODE_BACK;
    case pr::cull_mode::front:
        return D3D12_CULL_MODE_FRONT;
    }
}

}