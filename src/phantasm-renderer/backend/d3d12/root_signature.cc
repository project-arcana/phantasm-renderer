#include "root_signature.hh"

#include <phantasm-renderer/backend/d3d12/common/verify.hh>

pr::backend::d3d12::shared_com_ptr<ID3D12RootSignature> pr::backend::d3d12::create_root_signature(ID3D12Device& device,
                                                                                               cc::span<const CD3DX12_ROOT_PARAMETER> root_params,
                                                                                               cc::span<const CD3DX12_STATIC_SAMPLER_DESC> samplers)
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

void pr::backend::d3d12::root_signature::initialize(ID3D12Device &device, cc::span<const root_sig_payload_size> payload_sizes)
{
    detail::root_signature_params parameters;

    for (auto const& s : payload_sizes)
    {
        _payload_maps.push_back(parameters.add_payload_sizes(s));
    }

    raw_root_sig = create_root_signature(device, parameters.root_params, parameters.samplers);
}

void pr::backend::d3d12::root_signature::bind(ID3D12Device &device, ID3D12GraphicsCommandList &command_list, DynamicBufferRing &dynamic_buffer_ring, DescriptorManager &desc_manager, int payload_index, cc::span<const cpu_cbv_srv_uav> shader_resources, void *constant_buffer_data, void *root_constants_data)
{
    auto const& map = _payload_maps[payload_index];
    auto root_index = map.base_root_param;

    if (!shader_resources.empty())
    {
        // descriptor table
        auto desc_table = desc_manager.allocDynamicTable(device, shader_resources);
        command_list.SetGraphicsRootDescriptorTable(root_index++, desc_table.shader_resource_handle_gpu);
    }

    if (constant_buffer_data)
    {
        // root CBV

        void* cb_cpu;
        D3D12_GPU_VIRTUAL_ADDRESS cb_gpu;
        dynamic_buffer_ring.allocConstantBuffer(map.root_cbv_size_bytes, cb_cpu, cb_gpu);
        ::memcpy(cb_cpu, constant_buffer_data, map.root_cbv_size_bytes);

        command_list.SetGraphicsRootConstantBufferView(root_index++, cb_gpu);
    }

    if (root_constants_data)
    {
        // root constants
        command_list.SetGraphicsRoot32BitConstants(root_index++, map.num_root_constant_dwords, root_constants_data, 0);
    }
}

pr::backend::d3d12::root_sig_payload_map pr::backend::d3d12::detail::root_signature_params::add_payload_sizes(const pr::backend::d3d12::root_sig_payload_size &size)
{
    auto const res_index = unsigned(root_params.size());

    if (size.num_cbvs + size.num_srvs + size.num_uavs > 0)
        create_descriptor_table(size.num_cbvs, size.num_srvs, size.num_uavs);

    if (size.root_cbv_size_bytes > 0)
        create_root_cbv();

    auto const num_constant_dwords = unsigned(size.root_constants_size_bytes / sizeof(DWORD32));
    if (size.root_constants_size_bytes > 0)
        create_root_constants(num_constant_dwords);

    return {res_index, size.root_cbv_size_bytes, num_constant_dwords};
}

void pr::backend::d3d12::detail::root_signature_params::create_descriptor_table(int num_cbvs, int num_srvs, int num_uavs)
{
    auto const desc_range_start = desc_ranges.size();

    if (num_cbvs > 0)
    {
        desc_ranges.emplace_back();
        desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, num_cbvs, register_b);
        register_b += num_cbvs;
    }

    if (num_srvs > 0)
    {
        desc_ranges.emplace_back();
        desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, num_srvs, register_t);
        register_t += num_srvs;
    }

    if (num_uavs > 0)
    {
        desc_ranges.emplace_back();
        desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, num_uavs, register_u);
        register_u += num_uavs;
    }

    auto const desc_range_end = desc_ranges.size();

    // descriptor tables cost 1 dword
    ++size_dwords;

    CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
    param.InitAsDescriptorTable(UINT(desc_range_end - desc_range_start), desc_ranges.data() + desc_range_start, D3D12_SHADER_VISIBILITY_PIXEL);

    // implicitly create a sampler (TODO)
    // static samplers cost no dwords towards this size
    CD3DX12_STATIC_SAMPLER_DESC& sampler = samplers.emplace_back();
    sampler.Init(register_s++);
}

void pr::backend::d3d12::detail::root_signature_params::create_root_uav()
{
    CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();

    // root descriptors cost 2 dwords
    size_dwords += 2;

    param.InitAsUnorderedAccessView(register_u++, 0, D3D12_SHADER_VISIBILITY_PIXEL);
}

void pr::backend::d3d12::detail::root_signature_params::create_root_cbv()
{
    CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();

    // root descriptors cost 2 dwords
    size_dwords += 2;

    param.InitAsConstantBufferView(register_b++, 0, D3D12_SHADER_VISIBILITY_VERTEX);
}

void pr::backend::d3d12::detail::root_signature_params::create_root_constants(unsigned num_dwords)
{
    CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();

    // root constants cost their size in dowrds
    size_dwords += num_dwords;

    param.InitAsConstants(num_dwords, register_b++, 0, D3D12_SHADER_VISIBILITY_VERTEX);
}
