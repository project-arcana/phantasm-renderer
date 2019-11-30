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

[[nodiscard]] inline constexpr D3D12_COMMAND_LIST_TYPE to_native(queue_type type)
{
    switch (type)
    {
    case queue_type::graphics:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case queue_type::copy:
        return D3D12_COMMAND_LIST_TYPE_COPY;
    case queue_type::compute:
        return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    }
}

[[nodiscard]] inline constexpr D3D12_SRV_DIMENSION to_native_srv_dim(shader_view_dimension sv_dim)
{
    switch (sv_dim)
    {
    case shader_view_dimension::buffer:
        return D3D12_SRV_DIMENSION_BUFFER;
    case shader_view_dimension::texture1d:
        return D3D12_SRV_DIMENSION_TEXTURE1D;
    case shader_view_dimension::texture1d_array:
        return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
    case shader_view_dimension::texture2d:
        return D3D12_SRV_DIMENSION_TEXTURE2D;
    case shader_view_dimension::texture2d_ms:
        return D3D12_SRV_DIMENSION_TEXTURE2DMS;
    case shader_view_dimension::texture2d_array:
        return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    case shader_view_dimension::texture2d_ms_array:
        return D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
    case shader_view_dimension::texture3d:
        return D3D12_SRV_DIMENSION_TEXTURE3D;
    case shader_view_dimension::texturecube:
        return D3D12_SRV_DIMENSION_TEXTURECUBE;
    case shader_view_dimension::texturecube_array:
        return D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
    case shader_view_dimension::raytracing_accel_struct:
        return D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    }
}

[[nodiscard]] inline constexpr D3D12_UAV_DIMENSION to_native_uav_dim(shader_view_dimension sv_dim)
{
    switch (sv_dim)
    {
    case shader_view_dimension::buffer:
        return D3D12_UAV_DIMENSION_BUFFER;
    case shader_view_dimension::texture1d:
        return D3D12_UAV_DIMENSION_TEXTURE1D;
    case shader_view_dimension::texture1d_array:
        return D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
    case shader_view_dimension::texture2d:
        return D3D12_UAV_DIMENSION_TEXTURE2D;
    case shader_view_dimension::texture2d_array:
        return D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    case shader_view_dimension::texture3d:
        return D3D12_UAV_DIMENSION_TEXTURE3D;
    default:
        return D3D12_UAV_DIMENSION_UNKNOWN;
    }
}

[[nodiscard]] inline constexpr bool is_valid_as_uav_dim(shader_view_dimension sv_dim)
{
    return to_native_uav_dim(sv_dim) != D3D12_UAV_DIMENSION_UNKNOWN;
}

}
