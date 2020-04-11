#pragma once

#include <phantasm-hardware-interface/arguments.hh>
#include <phantasm-hardware-interface/detail/trivial_capped_vector.hh>
#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/murmur_hash.hh>


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
    murmur_hash hash;
};

// for economic reasons, SRVs, UAVs and Samplers are limited for cache-access shader views
struct shader_view_info
{
    phi::detail::trivial_capped_vector<phi::resource_view, 4> srvs;
    phi::detail::trivial_capped_vector<phi::resource_view, 4> uavs;
    phi::detail::trivial_capped_vector<uint64_t, 4> srv_guids;
    phi::detail::trivial_capped_vector<uint64_t, 4> uav_guids;
    phi::detail::trivial_capped_vector<phi::sampler_config, 2> samplers;
};

struct graphics_pass_info_data
{
    phi::pipeline_config graphics_config = {};
    unsigned vertex_size_bytes = 0;
    bool has_root_consts = false;
    phi::detail::trivial_capped_vector<phi::vertex_attribute_info, 8> vertex_attributes;
    phi::detail::trivial_capped_vector<phi::arg::shader_arg_shape, phi::limits::max_shader_arguments> arg_shapes;
    phi::detail::trivial_capped_vector<murmur_hash, 5> shader_hashes;
};

struct compute_pass_info_data
{
    bool has_root_consts = false;
    phi::detail::trivial_capped_vector<phi::arg::shader_arg_shape, phi::limits::max_shader_arguments> arg_shapes;
    murmur_hash shader_hash;
};
}
