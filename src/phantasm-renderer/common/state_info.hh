#pragma once

#include <clean-core/typedefs.hh>

#include <phantasm-hardware-interface/arguments.hh>
#include <phantasm-hardware-interface/common/container/flat_vector.hh>
#include <phantasm-hardware-interface/types.hh>


namespace pr
{
struct freeable_cached_obj
{
    enum type
    {
        graphics_sv,
        compute_sv,
        graphics_pso,
        compute_pso
    };

    type type;
    cc::hash_t hash;
};

// for economic reasons, SRVs, UAVs and Samplers are limited for cache-access shader views
struct shader_view_info
{
    phi::flat_vector<phi::resource_view, 4> srvs;
    phi::flat_vector<phi::resource_view, 4> uavs;
    phi::flat_vector<uint64_t, 4> srv_guids;
    phi::flat_vector<uint64_t, 4> uav_guids;
    phi::flat_vector<phi::sampler_config, 2> samplers;
};

struct graphics_pass_info_data
{
    phi::pipeline_config graphics_config = {};
    unsigned vertex_size_bytes = 0;
    bool has_root_consts = false;
    phi::flat_vector<phi::vertex_attribute_info, 8> vertex_attributes;
    phi::flat_vector<phi::arg::shader_arg_shape, phi::limits::max_shader_arguments> arg_shapes;
    phi::flat_vector<cc::hash_t, phi::limits::num_graphics_shader_stages> shader_hashes;
};

struct compute_pass_info_data
{
    bool has_root_consts = false;
    phi::flat_vector<phi::arg::shader_arg_shape, phi::limits::max_shader_arguments> arg_shapes;
    cc::hash_t shader_hash;
};
}
