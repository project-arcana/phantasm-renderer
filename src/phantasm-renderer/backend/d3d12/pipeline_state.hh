#pragma once

#include <clean-core/span.hh>
#include <clean-core/capped_vector.hh>

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

[[nodiscard]] shared_com_ptr<ID3D12PipelineState> create_pipeline_state(ID3D12Device& device,
                                                                        cc::span<D3D12_INPUT_ELEMENT_DESC const> input_layout,
                                                                        cc::span<shader const> shaders,
                                                                        ID3D12RootSignature* root_sig,
                                                                        pr::primitive_pipeline_config const& config,
                                                                        wip::framebuffer_format const& framebuffer);

}