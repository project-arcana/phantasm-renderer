#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>

#include "resources/resource_view.hh"
#include "shader.hh"

namespace pr::backend::d3d12
{
namespace wip
{
struct framebuffer_format
{
    cc::capped_vector<DXGI_FORMAT, 6> render_targets;
    cc::capped_vector<DXGI_FORMAT, 1> depth_target; // a poor man's optional
};
}

[[nodiscard]] ID3D12PipelineState* create_pipeline_state_ll(ID3D12Device& device,
                                                            ID3D12RootSignature* root_sig,
                                                            cc::span<D3D12_INPUT_ELEMENT_DESC const> vertex_input_layout,
                                                            arg::framebuffer_format framebuffer_format,
                                                            arg::shader_stages shader_stages,
                                                            pr::primitive_pipeline_config const& config);

[[nodiscard]] shared_com_ptr<ID3D12PipelineState> create_pipeline_state(ID3D12Device& device,
                                                                        cc::span<D3D12_INPUT_ELEMENT_DESC const> input_layout,
                                                                        cc::span<shader const> shaders,
                                                                        ID3D12RootSignature* root_sig,
                                                                        pr::primitive_pipeline_config const& config,
                                                                        wip::framebuffer_format const& framebuffer);

}
