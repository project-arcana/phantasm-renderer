#include "pipeline_state.hh"

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>
#include <phantasm-renderer/backend/d3d12/common/native_enum.hh>

ID3D12PipelineState* pr::backend::d3d12::create_pipeline_state(ID3D12Device& device,
                                                                  ID3D12RootSignature* root_sig,
                                                                  cc::span<const D3D12_INPUT_ELEMENT_DESC> vertex_input_layout,
                                                                  pr::backend::arg::framebuffer_format framebuffer_format,
                                                                  pr::backend::arg::shader_stages shader_stages,
                                                                  const pr::primitive_pipeline_config& config)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.InputLayout = {!vertex_input_layout.empty() ? vertex_input_layout.data() : nullptr, UINT(vertex_input_layout.size())};
    pso_desc.pRootSignature = root_sig;

    constexpr auto const to_bytecode = [](arg::shader_stage const& s) { return D3D12_SHADER_BYTECODE{s.binary_data, s.binary_size}; };

    for (arg::shader_stage const& s : shader_stages)
    {
        switch (s.domain)
        {
        case shader_domain::pixel:
            pso_desc.PS = to_bytecode(s);
            break;
        case shader_domain::vertex:
            pso_desc.VS = to_bytecode(s);
            break;
        case shader_domain::domain:
            pso_desc.DS = to_bytecode(s);
            break;
        case shader_domain::hull:
            pso_desc.HS = to_bytecode(s);
            break;
        case shader_domain::geometry:
            pso_desc.GS = to_bytecode(s);
            break;
        case shader_domain::compute:
            break;
        }
    }

    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso_desc.RasterizerState.CullMode = util::to_native(config.cull);
    pso_desc.RasterizerState.FrontCounterClockwise = true;

    pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso_desc.DepthStencilState.DepthEnable = config.depth != pr::depth_function::none && !framebuffer_format.depth_target.empty();
    pso_desc.DepthStencilState.DepthFunc = util::to_native(config.depth);
    pso_desc.DepthStencilState.DepthWriteMask = config.depth_readonly ? D3D12_DEPTH_WRITE_MASK_ZERO : D3D12_DEPTH_WRITE_MASK_ALL;

    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = util::to_native(config.topology);

    pso_desc.NumRenderTargets = UINT(framebuffer_format.render_targets.size());

    for (auto i = 0u; i < framebuffer_format.render_targets.size(); ++i)
        pso_desc.RTVFormats[i] = util::to_dxgi_format(framebuffer_format.render_targets[i]);

    pso_desc.DSVFormat = pso_desc.DepthStencilState.DepthEnable ? util::to_dxgi_format(framebuffer_format.depth_target[0]) : DXGI_FORMAT_UNKNOWN;

    pso_desc.SampleDesc.Count = UINT(config.samples);
    pso_desc.NodeMask = 0;
    pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    ID3D12PipelineState* pso;
    PR_D3D12_VERIFY(device.CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso)));
    return pso;
}
