#include "root_signature.hh"

#include <phantasm-renderer/backend/d3d12/common/verify.hh>

pr::backend::d3d12::shared_com_ptr<ID3D12RootSignature> pr::backend::d3d12::get_root_signature(ID3D12Device &device, cc::span<const CD3DX12_ROOT_PARAMETER> root_params, cc::span<const CD3DX12_STATIC_SAMPLER_DESC> samplers)
{
    CD3DX12_ROOT_SIGNATURE_DESC desc = {};
    desc.pParameters = root_params.data();
    desc.NumParameters = UINT(root_params.size());
    desc.pStaticSamplers = samplers.data();
    desc.NumStaticSamplers = UINT(samplers.size());
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    shared_com_ptr<ID3DBlob> serialized_root_sig;
    PR_D3D12_VERIFY(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, serialized_root_sig.override(), nullptr));

    shared_com_ptr<ID3D12RootSignature> res;
    PR_D3D12_VERIFY(device.CreateRootSignature(0, serialized_root_sig->GetBufferPointer(), serialized_root_sig->GetBufferSize(), PR_COM_WRITE(res)));
    return res;
}
