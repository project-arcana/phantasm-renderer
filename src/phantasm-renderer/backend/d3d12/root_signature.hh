#pragma once

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/unique_buffer.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>

namespace pr::backend::d3d12
{
struct shader_argument_map
{
    unsigned cbv_param;
    unsigned srv_uav_table_param;
    unsigned sampler_table_param;
};

namespace detail
{
/// allows constructive creation of a root signature by combining shader argument shapes
struct root_signature_params
{
    cc::capped_vector<CD3DX12_ROOT_PARAMETER, 16> root_params;
    cc::capped_vector<CD3DX12_STATIC_SAMPLER_DESC, 16> samplers;

    [[nodiscard]] shader_argument_map add_shader_argument_shape(arg::shader_argument_shape const& shape);
    void add_static_sampler(sampler_config const& config);

private:
    unsigned _space = 0;
    cc::capped_vector<CD3DX12_DESCRIPTOR_RANGE, 16> _desc_ranges;
};
}

/// creates a root signature from parameters and samplers
[[nodiscard]] ID3D12RootSignature* create_root_signature(ID3D12Device& device,
                                                         cc::span<CD3DX12_ROOT_PARAMETER const> root_params,
                                                         cc::span<CD3DX12_STATIC_SAMPLER_DESC const> samplers,
                                                         bool is_compute);

struct root_signature
{
    ID3D12RootSignature* raw_root_sig;
    cc::capped_vector<shader_argument_map, 4> argument_maps;
};

void initialize_root_signature(root_signature& root_sig, ID3D12Device& device, arg::shader_argument_shapes payload_shape, bool add_fixed_root_constants, bool is_compute);
}
