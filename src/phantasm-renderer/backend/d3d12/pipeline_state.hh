#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include "shader.hh"

namespace pr::backend::d3d12
{
[[nodiscard]] inline constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE pr_to_native(pr::primitive_topology topology)
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

[[nodiscard]] inline constexpr D3D12_PRIMITIVE_TOPOLOGY pr_to_native_topology(pr::primitive_topology topology)
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

[[nodiscard]] inline constexpr D3D12_COMPARISON_FUNC pr_to_native(pr::depth_function depth_func)
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

[[nodiscard]] inline constexpr D3D12_CULL_MODE pr_to_native(pr::cull_mode cull_mode)
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

[[nodiscard]] ID3D12PipelineState* create_pipeline_state(ID3D12Device& device,
                                                            ID3D12RootSignature* root_sig,
                                                            cc::span<D3D12_INPUT_ELEMENT_DESC const> vertex_input_layout,
                                                            arg::framebuffer_format framebuffer_format,
                                                            arg::shader_stages shader_stages,
                                                            pr::primitive_pipeline_config const& config);

}
