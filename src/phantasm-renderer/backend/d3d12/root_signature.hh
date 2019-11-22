#pragma once

#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
#include <phantasm-renderer/backend/detail/unique_buffer.hh>
#include <phantasm-renderer/backend/types.hh>

#include "resources/resource_creation.hh"

namespace pr::backend::d3d12
{
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

    [[nodiscard]] shader_argument_map add_shader_argument_shape(arg::shader_argument_shape const& shape);
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

[[nodiscard]] ID3D12RootSignature* create_root_signature_raw(ID3D12Device& device,
                                                             cc::span<CD3DX12_ROOT_PARAMETER const> root_params,
                                                             cc::span<CD3DX12_STATIC_SAMPLER_DESC const> samplers);

struct root_signature_ll
{
    ID3D12RootSignature* raw_root_sig;
    cc::capped_vector<shader_argument_map, 4> argument_maps;
};

inline void initialize_root_signature(root_signature_ll& root_sig, ID3D12Device& device, arg::shader_argument_shapes payload_shape)
{
    detail::root_signature_params parameters;

    for (auto const& arg_shape : payload_shape)
    {
        root_sig.argument_maps.push_back(parameters.add_shader_argument_shape(arg_shape));
    }

    parameters.add_implicit_sampler();
    root_sig.raw_root_sig = create_root_signature_raw(device, parameters.root_params, parameters.samplers);
}

inline void bind_root_signature_argument(root_signature_ll& root_sig,
                                         ID3D12GraphicsCommandList& cmd_list,
                                         int arg_index,
                                         D3D12_GPU_VIRTUAL_ADDRESS cbv_va,
                                         unsigned cbv_offset,
                                         D3D12_GPU_DESCRIPTOR_HANDLE descriptor_table)
{
    auto const& map = root_sig.argument_maps[static_cast<unsigned>(arg_index)];

    if (map.cbv_param != uint32_t(-1))
        cmd_list.SetGraphicsRootConstantBufferView(map.cbv_param, cbv_va + cbv_offset);

    if (map.descriptor_table_param != uint32_t(-1))
        cmd_list.SetGraphicsRootDescriptorTable(map.descriptor_table_param, descriptor_table);
}
}
