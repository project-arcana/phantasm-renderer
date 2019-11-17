#pragma once

#include <algorithm>
#include <type_traits>

#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/detail/unique_buffer.hh>
#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/immediate.hh>
#include <phantasm-renderer/resources.hh>

#include "memory/DynamicBufferRing.hh"
#include "resources/resource_creation.hh"

namespace pr::backend::d3d12
{
namespace wip
{
// these aliases are just bandaids to enable introspection helpers right now, eventually those would use proper pr types
struct srv : cpu_cbv_srv_uav
{
};

struct uav : cpu_cbv_srv_uav
{
};

struct cbv : cpu_cbv_srv_uav
{
};
}

// A shader payload is an array of arguments
struct shader_payload_shape
{
    // A shader argument consists of n SRVs, m UAVs plus a CBV and an offset into it
    struct shader_argument_shape
    {
        bool has_cb = false;
        unsigned num_srvs = 0;
        unsigned num_uavs = 0;
    };

    static constexpr auto max_num_arguments = 4;
    cc::capped_vector<shader_argument_shape, max_num_arguments> shader_arguments;
};

struct shader_argument
{
    D3D12_GPU_VIRTUAL_ADDRESS cbv_va = 0;
    uint32_t cbv_view_offset = 0;
    cc::capped_vector<cpu_cbv_srv_uav, 8> srvs;
    cc::capped_vector<cpu_cbv_srv_uav, 8> uavs;
};

struct shader_argument_map
{
    unsigned cbv_param;
    unsigned descriptor_table_param;
};


namespace detail
{
/// allows constructive creation of a root signature by combining payload sizes
struct root_signature_params
{
    cc::capped_vector<CD3DX12_ROOT_PARAMETER, 16> root_params;
    cc::capped_vector<CD3DX12_STATIC_SAMPLER_DESC, 16> samplers;

    [[nodiscard]] shader_argument_map add_shader_argument_shape(shader_payload_shape::shader_argument_shape const& shape);
    void add_implicit_sampler();

private:
    unsigned _space = 0;

private:
    cc::capped_vector<CD3DX12_DESCRIPTOR_RANGE, 16> desc_ranges;
};
}

/// creates a root signature from parameters and samplers
[[nodiscard]] shared_com_ptr<ID3D12RootSignature> create_root_signature(ID3D12Device& device,
                                                                        cc::span<CD3DX12_ROOT_PARAMETER const> root_params,
                                                                        cc::span<CD3DX12_STATIC_SAMPLER_DESC const> samplers);

struct root_signature
{
    /// Creates a root signature from its "shape", an array of payload sizes
    void initialize(ID3D12Device& device, shader_payload_shape const& payload_sizes);

    /// binds a shader argument
    void bind(ID3D12Device& device, ID3D12GraphicsCommandList& command_list, DescriptorAllocator& desc_manager, int argument_index, shader_argument const& argument);

    shared_com_ptr<ID3D12RootSignature> raw_root_sig;

private:
    cc::capped_vector<shader_argument_map, 8> _payload_maps;
};

}
