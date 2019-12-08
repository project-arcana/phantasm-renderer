#pragma once

// cc::hash does not have support for enums yet
#include <functional>

#include <clean-core/forward.hh>
#include <clean-core/hash.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

namespace pr::backend::hash
{
namespace detail
{
template <class T>
size_t hash(T const& a)
{
    return std::hash<T>{}(a);
}
template <class T1, class T2>
size_t hash(T1 const& a, T2 const& b)
{
    return cc::hash_combine(std::hash<T1>{}(a), std::hash<T2>{}(b));
}
template <class T1, class T2, class T3>
size_t hash(T1 const& a, T2 const& b, T3 const& c)
{
    return cc::hash_combine(std::hash<T1>{}(a), std::hash<T2>{}(b), std::hash<T3>{}(c));
}
template <class T1, class T2, class T3, class T4>
size_t hash(T1 const& a, T2 const& b, T3 const& c, T4 const& d)
{
    return cc::hash_combine(std::hash<T1>{}(a), std::hash<T2>{}(b), std::hash<T3>{}(c), std::hash<T4>{}(d));
}
}

inline size_t compute(arg::shader_argument_shape const& v) { return cc::make_hash(v.num_srvs, v.num_uavs, v.num_samplers, v.has_cb); }

inline size_t compute(arg::shader_argument_shapes const v)
{
    size_t res = 0;
    for (auto const& e : v)
        res = cc::hash_combine(res, compute(e));
    return res;
}

inline size_t compute(arg::framebuffer_format const v)
{
    size_t res = 0;
    for (auto const f_rt : v.render_targets)
        res = cc::hash_combine(res, detail::hash(f_rt));
    for (auto const f_ds : v.depth_target)
        res = cc::hash_combine(res, detail::hash(f_ds));
    return res;
}


inline size_t compute(shader_argument const& v) { return cc::make_hash(v.constant_buffer.index, v.constant_buffer_offset, v.shader_view.index); }

inline size_t compute(pr::primitive_pipeline_config const& v)
{
    return cc::hash_combine(detail::hash(v.topology, v.depth, v.depth_readonly, v.cull), detail::hash(v.samples));
}

inline size_t compute(sampler_config const& v)
{
    return cc::hash_combine(                                              //
        detail::hash(v.filter, v.address_u, v.address_v, v.address_w),    //
        detail::hash(v.min_lod, v.max_lod, v.lod_bias, v.max_anisotropy), //
        detail::hash(v.compare_func, v.border_color)                      //

    );
}

struct compute_functor
{
    template <class KeyT>
    std::size_t operator()(KeyT&& v) const noexcept
    {
        return compute(cc::forward<KeyT&&>(v));
    }
};

}
