#include "pipeline_state.hh"

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

namespace
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
}

pr::backend::d3d12::shared_com_ptr<ID3D12PipelineState> pr::backend::d3d12::create_pipeline_state(ID3D12Device& device,
                                                                                                  cc::span<D3D12_INPUT_ELEMENT_DESC const> input_layout,
                                                                                                  cc::span<shader const> shaders,
                                                                                                  ID3D12RootSignature* root_sig,
                                                                                                  const pr::primitive_pipeline_config& config)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {}; // explicit init on purpose
    pso_desc.InputLayout = {input_layout.data(), UINT(input_layout.size())};
    pso_desc.pRootSignature = root_sig;

    for (shader const& s : shaders)
    {
        switch (s.domain)
        {
        case shader_domain::pixel:
            pso_desc.PS = {s.blob->GetBufferPointer(), s.blob->GetBufferSize()};
            break;
        case shader_domain::vertex:
            pso_desc.VS = {s.blob->GetBufferPointer(), s.blob->GetBufferSize()};
            break;
        case shader_domain::domain:
            pso_desc.DS = {s.blob->GetBufferPointer(), s.blob->GetBufferSize()};
            break;
        case shader_domain::hull:
            pso_desc.HS = {s.blob->GetBufferPointer(), s.blob->GetBufferSize()};
            break;
        case shader_domain::geometry:
            pso_desc.GS = {s.blob->GetBufferPointer(), s.blob->GetBufferSize()};
            break;
        case shader_domain::compute:
            break;
        }
    }

    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso_desc.RasterizerState.CullMode = pr_to_native(config.cull);
    pso_desc.RasterizerState.FrontCounterClockwise = true;

    pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso_desc.DepthStencilState.DepthEnable = config.depth != pr::depth_function::none;
    pso_desc.DepthStencilState.DepthFunc = pr_to_native(config.depth);

    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = pr_to_native(config.topology);

    // TODO
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // backend.mSwapchain.getBackbufferFormat();
    pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;          // TODO store this

    pso_desc.SampleDesc.Count = UINT(config.samples);
    pso_desc.NodeMask = 0;
    pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    shared_com_ptr<ID3D12PipelineState> pso;
    PR_D3D12_VERIFY(device.CreateGraphicsPipelineState(&pso_desc, PR_COM_WRITE(pso)));
    return pso;
}
