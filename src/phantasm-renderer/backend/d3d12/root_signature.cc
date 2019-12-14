#include "root_signature.hh"

#include <iostream>

#include <phantasm-renderer/backend/d3d12/common/native_enum.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

ID3D12RootSignature* pr::backend::d3d12::create_root_signature(ID3D12Device& device,
                                                               cc::span<const CD3DX12_ROOT_PARAMETER> root_params,
                                                               cc::span<const CD3DX12_STATIC_SAMPLER_DESC> samplers,
                                                               bool is_compute)
{
    CD3DX12_ROOT_SIGNATURE_DESC desc = {};
    desc.pParameters = root_params.empty() ? nullptr : root_params.data();
    desc.NumParameters = UINT(root_params.size());
    desc.pStaticSamplers = samplers.empty() ? nullptr : samplers.data();
    desc.NumStaticSamplers = UINT(samplers.size());
    if (is_compute)
    {
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
                     | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    }
    else
    {
        desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        //            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
        //                 | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
    }

    shared_com_ptr<ID3DBlob> serialized_root_sig;
    shared_com_ptr<ID3DBlob> error_blob;
    auto const serialize_hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, serialized_root_sig.override(), error_blob.override());
    if (serialize_hr == E_INVALIDARG)
    {
        std::cerr << "[pr][backend][d3d12] Root signature serialization failed: \n  " << static_cast<char*>(error_blob->GetBufferPointer()) << std::endl;
    }
    PR_D3D12_ASSERT(serialize_hr);


    ID3D12RootSignature* res;
    PR_D3D12_VERIFY(device.CreateRootSignature(0, serialized_root_sig->GetBufferPointer(), serialized_root_sig->GetBufferSize(), IID_PPV_ARGS(&res)));
    return res;
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
        auto const desc_range_start = _desc_ranges.size();

        if (shape.num_srvs > 0)
        {
            _desc_ranges.emplace_back();
            _desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, shape.num_srvs, 0, _space);
        }

        if (shape.num_uavs > 0)
        {
            _desc_ranges.emplace_back();
            _desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, shape.num_uavs, 0, _space);
        }

        auto const desc_range_end = _desc_ranges.size();

        CD3DX12_ROOT_PARAMETER& desc_table = root_params.emplace_back();
        desc_table.InitAsDescriptorTable(UINT(desc_range_end - desc_range_start), _desc_ranges.data() + desc_range_start, argument_visibility);
        res_map.srv_uav_table_param = uint32_t(root_params.size() - 1);
    }
    else
    {
        res_map.srv_uav_table_param = uint32_t(-1);
    }

    if (shape.num_samplers > 0)
    {
        auto const desc_range_start = _desc_ranges.size();
        _desc_ranges.emplace_back();
        _desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, shape.num_samplers, 0, _space);

        auto const desc_range_end = _desc_ranges.size();

        CD3DX12_ROOT_PARAMETER& desc_table = root_params.emplace_back();
        desc_table.InitAsDescriptorTable(UINT(desc_range_end - desc_range_start), _desc_ranges.data() + desc_range_start, argument_visibility);
        res_map.sampler_table_param = uint32_t(root_params.size() - 1);
    }
    else
    {
        res_map.sampler_table_param = uint32_t(-1);
    }

    ++_space;
    return res_map;
}

void pr::backend::d3d12::detail::root_signature_params::add_static_sampler(const sampler_config& config)
{
    CD3DX12_STATIC_SAMPLER_DESC& sampler = samplers.emplace_back();
    sampler.Init(static_cast<UINT>(samplers.size() - 1),                                                //
                 util::to_native(config.filter, config.compare_func != sampler_compare_func::disabled), //
                 util::to_native(config.address_u),                                                     //
                 util::to_native(config.address_v),                                                     //
                 util::to_native(config.address_w),                                                     //
                 config.lod_bias,                                                                       //
                 config.max_anisotropy,                                                                 //
                 util::to_native(config.compare_func),                                                  //
                 util::to_native(config.border_color),                                                  //
                 config.min_lod,                                                                        //
                 config.max_lod,                                                                        //
                 D3D12_SHADER_VISIBILITY_ALL,                                                           //
                 0                                                                                      // space 0
    );
}

void pr::backend::d3d12::initialize_root_signature(pr::backend::d3d12::root_signature& root_sig,
                                                   ID3D12Device& device,
                                                   pr::backend::arg::shader_argument_shapes payload_shape,
                                                   bool add_fixed_root_constants,
                                                   bool is_compute)
{
    detail::root_signature_params parameters;

    for (auto const& arg_shape : payload_shape)
    {
        root_sig.argument_maps.push_back(parameters.add_shader_argument_shape(arg_shape));
    }

    root_sig.raw_root_sig = create_root_signature(device, parameters.root_params, parameters.samplers, is_compute);
}
