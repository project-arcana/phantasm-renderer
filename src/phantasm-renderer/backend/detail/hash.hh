#pragma once

#include <clean-core/forward.hh>
#include <clean-core/hash.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

namespace pr::backend::hash
{
inline cc::hash_t compute(arg::shader_argument_shape const& v) { return cc::make_hash(v.num_srvs, v.num_uavs, v.num_samplers, v.has_cb); }

inline cc::hash_t compute(arg::shader_argument_shapes const v)
{
    cc::hash_t res = 0;
    for (auto const& e : v)
        res = cc::hash_combine(res, compute(e));
    return res;
}

inline cc::hash_t compute(shader_argument const& v) { return cc::make_hash(v.constant_buffer.index, v.constant_buffer_offset, v.shader_view.index); }

inline cc::hash_t compute(pr::primitive_pipeline_config const& v)
{
    static_assert(sizeof(pr::primitive_pipeline_config) == 20, "hash function out of date");
    return cc::hash_combine(cc::make_hash(v.topology, v.depth, v.depth_readonly, v.cull), cc::make_hash(v.samples));
}

inline cc::hash_t compute(sampler_config const& v)
{
    return cc::hash_combine(                                               //
        cc::make_hash(v.filter, v.address_u, v.address_v, v.address_w),    //
        cc::make_hash(v.min_lod, v.max_lod, v.lod_bias, v.max_anisotropy), //
        cc::make_hash(v.compare_func, v.border_color)                      //

    );
}

struct compute_functor
{
    template <class KeyT>
    cc::hash_t operator()(KeyT&& v) const noexcept
    {
        return compute(cc::forward<KeyT&&>(v));
    }
};

}
