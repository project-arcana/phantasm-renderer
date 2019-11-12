#pragma once

#include <algorithm>
#include <type_traits>

#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/immediate.hh>
#include <phantasm-renderer/resources.hh>

#include "memory/DynamicBufferRing.hh"
#include "resources/resource_creation.hh"

namespace pr::backend::d3d12
{
namespace wip
{
struct srv : cpu_cbv_srv_uav
{
};

struct uav : cpu_cbv_srv_uav
{
};
}

/// Describes the size of a payload
struct root_sig_payload_size
{
    int num_cbvs;
    int num_srvs;
    int num_uavs;
    unsigned root_cbv_size_bytes;
    unsigned root_constants_size_bytes;
};

/// Contains data to be bound
struct root_sig_payload_data
{
    cc::capped_vector<cpu_cbv_srv_uav, 16> resources;
    void* cbv_data;
    void* constant_data;

    void free()
    {
        if (cbv_data)
            ::free(cbv_data);
        if (constant_data)
            ::free(constant_data);
    }
};

/// Contains mapping data necessary to bind a root_sig_payload_data
struct root_sig_payload_map
{
    unsigned base_root_param;
    unsigned root_cbv_size_bytes;
    unsigned num_root_constant_dwords;
};

namespace detail
{
struct root_signature_params
{
    cc::capped_vector<CD3DX12_ROOT_PARAMETER, 16> root_params;
    cc::capped_vector<CD3DX12_STATIC_SAMPLER_DESC, 16> samplers;

    /// Adds a payload size into the signature description
    /// Returns the base root parameter index of the payload
    root_sig_payload_map add_payload_sizes(root_sig_payload_size const& size);

private:
    void create_descriptor_table(int num_cbvs, int num_srvs, int num_uavs);

    void create_root_uav();

    void create_root_cbv();

    void create_root_constants(unsigned num_dwords);

private:
    cc::capped_vector<CD3DX12_DESCRIPTOR_RANGE, 16> desc_ranges;

    unsigned register_b = 0;
    unsigned register_t = 0;
    unsigned register_u = 0;
    unsigned register_s = 0;

    unsigned size_dwords = 0;
};

struct data_size_measure_visitor
{
    root_sig_payload_size size = {};

    template <class T>
    void operator()(T const&, char const*)
    {
        if constexpr (std::is_same_v<T, wip::srv>)
        {
            ++size.num_srvs;
        }
        else if constexpr (std::is_same_v<T, wip::uav>)
        {
            ++size.num_uavs;
        }
        else if constexpr (std::is_base_of_v<pr::immediate, T>)
        {
            // pr::immediate, a root constant
            size.root_constants_size_bytes += sizeof(T);
        }
        else
        {
            // raw data, summed into a root CBV
            size.root_cbv_size_bytes += sizeof(T);
        }
    }
};

struct data_extraction_visitor
{
    root_sig_payload_data data;

    unsigned cbv_offset = 0;
    unsigned constants_offset = 0;

    data_extraction_visitor(unsigned cbv_size, unsigned constants_size)
    {
        data.cbv_data = cbv_size > 0 ? ::malloc(cbv_size) : nullptr;
        data.constant_data = constants_size > 0 ? ::malloc(constants_size) : nullptr;
    }

    template <class T>
    void operator()(T const& val, char const*)
    {
        if constexpr (std::is_same_v<T, wip::srv>)
        {
            data.resources.push_back(val);
        }
        else if constexpr (std::is_same_v<T, wip::uav>)
        {
            data.resources.push_back(val);
        }
        else if constexpr (std::is_base_of_v<pr::immediate, T>)
        {
            // pr::immediate, a root constant
            ::memcpy(static_cast<char*>(data.constant_data) + constants_offset, &val, sizeof(T));
            constants_offset += sizeof(T);
        }
        else
        {
            // raw data, summed into a root CBV
            ::memcpy(static_cast<char*>(data.cbv_data) + cbv_offset, &val, sizeof(T));
            cbv_offset += sizeof(T);
        }
    }
};
}

[[nodiscard]] shared_com_ptr<ID3D12RootSignature> create_root_signature(ID3D12Device& device,
                                                                        cc::span<CD3DX12_ROOT_PARAMETER const> root_params,
                                                                        cc::span<CD3DX12_STATIC_SAMPLER_DESC const> samplers);

template <class DataT>
[[nodiscard]] root_sig_payload_size get_payload_size()
{
    detail::data_size_measure_visitor visitor;
    DataT* volatile dummy_pointer = nullptr;
    introspect(visitor, *dummy_pointer);
    return visitor.size;
}

template <class DataT>
[[nodiscard]] root_sig_payload_data get_payload_data(DataT const& data, root_sig_payload_size const& size)
{
    detail::data_extraction_visitor visitor(size.root_cbv_size_bytes, size.root_constants_size_bytes);
    introspect(visitor, const_cast<DataT&>(data));
    return visitor.data;
}

struct root_signature
{
    void initialize(ID3D12Device& device, cc::span<root_sig_payload_size const> payload_sizes);

    void bind(ID3D12Device& device,
              ID3D12GraphicsCommandList& command_list,
              DynamicBufferRing& dynamic_buffer_ring,
              DescriptorAllocator& desc_manager,
              int payload_index,
              cc::span<cpu_cbv_srv_uav const> shader_resources,
              void* constant_buffer_data,
              void* root_constants_data);

    void bind(ID3D12Device& device,
              ID3D12GraphicsCommandList& command_list,
              DynamicBufferRing& dynamic_buffer_ring,
              DescriptorAllocator& desc_manager,
              int payload_index,
              root_sig_payload_data const& data)
    {
        bind(device, command_list, dynamic_buffer_ring, desc_manager, payload_index, data.resources, data.cbv_data, data.constant_data);
    }

    shared_com_ptr<ID3D12RootSignature> raw_root_sig;

private:
    cc::capped_vector<root_sig_payload_map, 8> _payload_maps;
};

}
