#pragma once

#include <type_traits>

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/immediate.hh>
#include <phantasm-renderer/resources.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>

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
template <class DataT>
struct data_visitor
{
    cc::capped_vector<CD3DX12_ROOT_PARAMETER, 8> root_params;
    cc::capped_vector<CD3DX12_STATIC_SAMPLER_DESC, 8> samplers;
    cc::capped_vector<CD3DX12_DESCRIPTOR_RANGE, 16> desc_ranges;

    unsigned register_b = 0;
    unsigned register_t = 0;
    unsigned register_u = 0;
    unsigned register_s = 0;

    template <class T>
    void operator()(T const&, char const* /*name*/)
    {
        if constexpr (std::is_same_v<T, wip::srv>)
        {
            // Implicitly create a descriptor table range (TODO: batching)
            {
                auto const desc_range_start = desc_ranges.size();

                desc_ranges.emplace_back();
                desc_ranges.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, register_t++);

                auto const desc_range_end = desc_ranges.size();

                CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
                param.InitAsDescriptorTable(UINT(desc_range_end - desc_range_start), desc_ranges.data() + desc_range_start, D3D12_SHADER_VISIBILITY_PIXEL);
            }

            //            // SRV, a root shader resource view
            //            CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
            //            param.InitAsShaderResourceView(register_t++, 0, D3D12_SHADER_VISIBILITY_PIXEL);

            // implicitly create a sampler (TODO)
            CD3DX12_STATIC_SAMPLER_DESC& sampler = samplers.emplace_back();
            sampler.Init(register_s++);
        }
        else if constexpr (std::is_same_v<T, wip::uav>)
        {
            // <TBD>, an unordered access view
            CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
            param.InitAsUnorderedAccessView(register_u++);
        }
        else if constexpr (std::is_base_of_v<wip::constant_buffer, T>)
        {
            // <TBD>, a constant buffer view
            CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
            param.InitAsConstantBufferView(register_b++, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        }
        else if constexpr (std::is_base_of_v<pr::immediate, T>)
        {
            // pr::immediate, a root constant
            auto constexpr num_dwords = sizeof(T) / sizeof(float);
            CD3DX12_ROOT_PARAMETER& param = root_params.emplace_back();
            param.InitAsConstants(num_dwords, register_b++, 0, D3D12_SHADER_VISIBILITY_VERTEX);
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

[[nodiscard]] shared_com_ptr<ID3D12RootSignature> get_root_signature(ID3D12Device& device,
                                                                     cc::span<CD3DX12_ROOT_PARAMETER const> root_params,
                                                                     cc::span<CD3DX12_STATIC_SAMPLER_DESC const> samplers);

template <class DataT>
[[nodiscard]] shared_com_ptr<ID3D12RootSignature> get_root_signature(ID3D12Device& device)
{
    detail::data_visitor<DataT> visitor;
    DataT* volatile dummy_pointer = nullptr; // Volatile to avoid UB-based optimization
    introspect(visitor, *dummy_pointer);
    return get_root_signature(device, visitor.root_params, visitor.samplers);
}

}
