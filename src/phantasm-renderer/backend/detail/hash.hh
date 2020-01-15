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

struct compute_functor
{
    template <class KeyT>
    cc::hash_t operator()(KeyT&& v) const noexcept
    {
        return compute(cc::forward<KeyT&&>(v));
    }
};

}
