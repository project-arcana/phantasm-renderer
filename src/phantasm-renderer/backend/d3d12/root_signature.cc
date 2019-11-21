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
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    //            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    //                 | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    shared_com_ptr<ID3DBlob> serialized_root_sig;
    PR_D3D12_VERIFY(D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, serialized_root_sig.override(), nullptr));

    shared_com_ptr<ID3D12RootSignature> res;
    PR_D3D12_VERIFY(device.CreateRootSignature(0, serialized_root_sig->GetBufferPointer(), serialized_root_sig->GetBufferSize(), PR_COM_WRITE(res)));
    return res;
}

void pr::backend::d3d12::root_signature_high_level::initialize(ID3D12Device& device, arg::shader_argument_shapes payload_shape)
{
    detail::root_signature_params parameters;

    for (auto const& arg_shape : payload_shape)
    {
        _payload_maps.push_back(parameters.add_shader_argument_shape(arg_shape));
    }

    parameters.add_implicit_sampler();

    raw_root_sig = create_root_signature(device, parameters.root_params, parameters.samplers);
}

void pr::backend::d3d12::root_signature_high_level::bind(
    ID3D12Device& device, ID3D12GraphicsCommandList& command_list, DescriptorAllocator& desc_allocator, int argument_index, const legacy::shader_argument& argument)
{
    auto const& map = _payload_maps[unsigned(argument_index)];


    if (map.cbv_param != uint32_t(-1))
    {
        // root cbv
        command_list.SetGraphicsRootConstantBufferView(map.cbv_param, argument.cbv_va + argument.cbv_view_offset);
    }

    if (map.descriptor_table_param != uint32_t(-1))
    {
        // descriptor table
        auto desc_table = desc_allocator.allocDynamicTable(device, {}, argument.srvs, argument.uavs);
        command_list.SetGraphicsRootDescriptorTable(map.descriptor_table_param, desc_table.shader_resource_handle_gpu);
    }
}

pr::backend::d3d12::shader_argument_map pr::backend::d3d12::detail::root_signature_params::add_shader_argument_shape(const pr::backend::arg::shader_argument_shape& shape)
{
    shader_argument_map res_map;
    auto const argument_visibility = D3D12_SHADER_VISIBILITY_ALL; // NOTE: Eventually arguments could be constrained to stages

    // create root descriptor to CBV
    if (shape.has_cb)
    {
        CD3DX12_ROOT_PARAMETER& root_cbv = root_params.emplace_back();
        root_cbv.InitAsConstantBufferView(0, _space, argument_visibility);
        res_map.cbv_param = uint32_t(root_params.size() - 1);
    }
    else
    {
        res_map.cbv_param = uint32_t(-1);
    }

    // create descriptor table for SRVs and UAVs
    if (shape.num_srvs + shape.num_uavs > 0)
    {
        auto const desc_range_start = desc_ranges.size();

        if (shape.num_srvs > 0)
        {
            desc_ranges.emplace_back();
            desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shape.num_srvs, 0, _space);
        }

        if (shape.num_uavs > 0)
        {
            desc_ranges.emplace_back();
            desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, shape.num_uavs, 0, _space);
        }

        auto const desc_range_end = desc_ranges.size();

        CD3DX12_ROOT_PARAMETER& desc_table = root_params.emplace_back();
        desc_table.InitAsDescriptorTable(UINT(desc_range_end - desc_range_start), desc_ranges.data() + desc_range_start, argument_visibility);
        res_map.descriptor_table_param = uint32_t(root_params.size() - 1);
    }
    else
    {
        res_map.descriptor_table_param = uint32_t(-1);
    }

    ++_space;
    return res_map;
}

void pr::backend::d3d12::detail::root_signature_params::add_implicit_sampler()
{
    // implicitly create a sampler (TODO)
    // static samplers cost no dwords towards this size
    // Note this sampler is created in space0
    CD3DX12_STATIC_SAMPLER_DESC& sampler = samplers.emplace_back();
    sampler.Init(0);
}


