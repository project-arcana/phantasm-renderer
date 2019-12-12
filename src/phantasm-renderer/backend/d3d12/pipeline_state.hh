#pragma once

#include <clean-core/span.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_fwd.hh>

namespace pr::backend::d3d12
{
[[nodiscard]] ID3D12PipelineState* create_pipeline_state(ID3D12Device& device,
                                                         ID3D12RootSignature* root_sig,
                                                         cc::span<D3D12_INPUT_ELEMENT_DESC const> vertex_input_layout,
                                                         arg::framebuffer_format framebuffer_format,
                                                         arg::shader_stages shader_stages,
                                                         pr::primitive_pipeline_config const& config);

[[nodiscard]] ID3D12PipelineState* create_compute_pipeline_state(ID3D12Device& device, ID3D12RootSignature* root_sig, arg::shader_stage const& compute_shader);
}
