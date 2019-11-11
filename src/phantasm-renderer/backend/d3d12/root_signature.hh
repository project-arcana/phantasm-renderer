#pragma once

#include <algorithm>
#include <type_traits>

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/immediate.hh>
#include <phantasm-renderer/resources.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>

#include "memory/DynamicBufferRing.hh"
#include "resources/resource_creation.hh"

namespace pr::backend::d3d12
{
namespace wip
{
struct srv : resource_view
{
};

struct uav : resource_view
{
};

struct constant_buffer
{
};

}

namespace detail
{
struct root_signature_data
{
    cc::capped_vector<CD3DX12_ROOT_PARAMETER, 16> root_params;
    cc::capped_vector<CD3DX12_STATIC_SAMPLER_DESC, 16> samplers;
    cc::capped_vector<CD3DX12_DESCRIPTOR_RANGE, 16> desc_ranges;

    unsigned register_b = 0;
    unsigned register_t = 0;
    unsigned register_u = 0;
    unsigned register_s = 0;

    int create_single_srv_descriptor_table()
    {
        auto const res_index = int(root_params.size());

        auto const desc_range_start = desc_ranges.size();

        desc_ranges.emplace_back();
        desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, register_t++);

        auto const desc_range_end = desc_ranges.size();

        CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
        param.InitAsDescriptorTable(UINT(desc_range_end - desc_range_start), desc_ranges.data() + desc_range_start, D3D12_SHADER_VISIBILITY_PIXEL);


        //            // SRV, a root shader resource view
        //            CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
        //            param.InitAsShaderResourceView(register_t++, 0, D3D12_SHADER_VISIBILITY_PIXEL);

        // implicitly create a sampler (TODO)
        CD3DX12_STATIC_SAMPLER_DESC& sampler = samplers.emplace_back();
        sampler.Init(register_s++);

        return res_index;
    }

    int create_root_uav()
    {
        auto const res_index = int(root_params.size());
        CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
        param.InitAsUnorderedAccessView(register_u++);
        return res_index;
    }

    int create_root_cbv()
    {
        auto const res_index = int(root_params.size());
        CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
        param.InitAsConstantBufferView(register_b++, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        return res_index;
    }

    int create_root_constants(unsigned num_dwords)
    {
        auto const res_index = int(root_params.size());
        CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
        param.InitAsConstants(num_dwords, register_b++, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        return res_index;
    }
};

struct resource_map
{
    cc::capped_vector<int, 16> param_order;
};


template <class DataT>
struct data_binder
{
    ID3D12GraphicsCommandList& command_list;
    DynamicBufferRing& dynamic_buffer_ring;
    resource_map const& map;
    unsigned current_param = 0;

    template <class T>
    void operator()(T const& val, char const*)
    {
        int const root_index = map.param_order[current_param++];

        if constexpr (std::is_same_v<T, wip::srv>)
        {
            // <TBD>, an implicitly created descriptor table with a single SRV
            command_list.SetGraphicsRootDescriptorTable(root_index, val.get_gpu());
        }
        else if constexpr (std::is_same_v<T, wip::uav>)
        {
            // <TBD>, an unordered access view
            command_list.SetGraphicsRootUnorderedAccessView(root_index, val.get_gpu());
        }
        else if constexpr (std::is_base_of_v<wip::constant_buffer, T>)
        {
            // <TBD>, a constant buffer view
            T* cb_cpu;
            D3D12_GPU_VIRTUAL_ADDRESS cb_gpu;
            dynamic_buffer_ring.allocConstantBuffer(cb_cpu, cb_gpu);
            *cb_cpu = val;

            command_list.SetGraphicsRootConstantBufferView(root_index, cb_gpu);
        }
        else if constexpr (std::is_base_of_v<pr::immediate, T>)
        {
            // pr::immediate, a root constant
            auto constexpr num_dwords = sizeof(T) / sizeof(float);
            command_list.SetGraphicsRoot32BitConstants(root_index, num_dwords, &val, 0);
        }
        else
        {
            static_assert(sizeof(T) == 0, "Unknown type");
        }
    }
};

template <class DataT>
struct data_visitor
{
    root_signature_data& root_sig_data;
    resource_map& map;

    template <class T>
    void operator()(T const&, char const* /*name*/)
    {
        if constexpr (std::is_same_v<T, wip::srv>)
        {
            map.param_order.push_back(root_sig_data.create_single_srv_descriptor_table());
        }
        else if constexpr (std::is_same_v<T, wip::uav>)
        {
            // <TBD>, an unordered access view
            map.param_order.push_back(root_sig_data.create_root_uav());
        }
        else if constexpr (std::is_base_of_v<wip::constant_buffer, T>)
        {
            // <TBD>, a constant buffer view
            map.param_order.push_back(root_sig_data.create_root_cbv());
        }
        else if constexpr (std::is_base_of_v<pr::immediate, T>)
        {
            // pr::immediate, a root constant
            auto constexpr num_dwords = sizeof(T) / sizeof(float);
            map.param_order.push_back(root_sig_data.create_root_constants(num_dwords));
        }
        else
        {
            static_assert(sizeof(T) == 0, "Unknown type");
        }

        //        {

        //            cc::capped_vector<CD3DX12_DESCRIPTOR_RANGE, 8> desc_ranges;
        //            desc_ranges.emplace_back();
        //            desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, register_t++);

        //            CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
        //            param.InitAsDescriptorTable(UINT(desc_ranges.size()), desc_ranges.data(), D3D12_SHADER_VISIBILITY_ALL);
        //        }
    }
};
}

[[nodiscard]] shared_com_ptr<ID3D12RootSignature> create_root_signature(ID3D12Device& device,
                                                                        cc::span<CD3DX12_ROOT_PARAMETER const> root_params,
                                                                        cc::span<CD3DX12_STATIC_SAMPLER_DESC const> samplers);

template <class DataT, class InstanceDataT>
[[nodiscard]] shared_com_ptr<ID3D12RootSignature> create_root_signature(ID3D12Device& device)
{
    detail::root_signature_data root_sig_data;
    detail::resource_map outer_map;
    detail::resource_map instance_map;

    {
        detail::data_visitor<DataT> visitor = {root_sig_data, outer_map};
        DataT* volatile dummy_pointer = nullptr; // Volatile to avoid UB-based optimization
        introspect(visitor, *dummy_pointer);
    }

    {
        detail::data_visitor<InstanceDataT> visitor = {root_sig_data, instance_map};
        InstanceDataT* volatile dummy_pointer = nullptr; // Volatile to avoid UB-based optimization
        introspect(visitor, *dummy_pointer);
    }

    return create_root_signature(device, root_sig_data.root_params, root_sig_data.samplers);
}


template <class PassDataT, class InstanceDataT>
struct root_signature
{
    void initialize(ID3D12Device& device)
    {
        {
            detail::data_visitor<PassDataT> visitor = {_root_sig_data, _pass_data_map};
            PassDataT* volatile dummy_pointer = nullptr; // Volatile to avoid UB-based optimization
            introspect(visitor, *dummy_pointer);
        }

        {
            detail::data_visitor<InstanceDataT> visitor = {_root_sig_data, _instance_data_map};
            InstanceDataT* volatile dummy_pointer = nullptr; // Volatile to avoid UB-based optimization
            introspect(visitor, *dummy_pointer);
        }

        raw_root_sig = create_root_signature(device, _root_sig_data.root_params, _root_sig_data.samplers);
    }

    void bind(ID3D12GraphicsCommandList& command_list, DynamicBufferRing& dynamic_buffer_ring, PassDataT& pass_data)
    {
        detail::data_binder<PassDataT> binder = {command_list, dynamic_buffer_ring, _pass_data_map};
        introspect(binder, pass_data);
    }

    void bind(ID3D12GraphicsCommandList& command_list, DynamicBufferRing& dynamic_buffer_ring, InstanceDataT& instance_data)
    {
        detail::data_binder<InstanceDataT> binder = {command_list, dynamic_buffer_ring, _instance_data_map};
        introspect(binder, instance_data);
    }

    shared_com_ptr<ID3D12RootSignature> raw_root_sig;

private:
    detail::root_signature_data _root_sig_data;
    detail::resource_map _pass_data_map;
    detail::resource_map _instance_data_map;
};

}
