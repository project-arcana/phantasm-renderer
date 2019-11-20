#pragma once

#include <utility>

#include <clean-core/forward.hh>

#include <phantasm-renderer/backend/arguments.hh>

namespace pr::backend::hash
{
namespace detail
{
inline size_t hash_combine(size_t a, size_t b) { return a * 6364136223846793005ULL + b + 0xda3e39cb94b95bdbULL; }
inline size_t hash_combine(size_t a, size_t b, size_t c) { return hash_combine(hash_combine(a, b), c); }
inline size_t hash_combine(size_t a, size_t b, size_t c, size_t d) { return hash_combine(hash_combine(a, b), hash_combine(c, d)); }

template <class T>
size_t hash(T const& a)
{
    return std::hash<T>{}(a);
}
template <class T1, class T2>
size_t hash(T1 const& a, T2 const& b)
{
    return hash_combine(std::hash<T1>{}(a), std::hash<T2>{}(b));
}
template <class T1, class T2, class T3>
size_t hash(T1 const& a, T2 const& b, T3 const& c)
{
    return hash_combine(std::hash<T1>{}(a), std::hash<T2>{}(b), std::hash<T3>{}(c));
}
template <class T1, class T2, class T3, class T4>
size_t hash(T1 const& a, T2 const& b, T3 const& c, T4 const& d)
{
    return hash_combine(std::hash<T1>{}(a), std::hash<T2>{}(b), std::hash<T3>{}(c), std::hash<T4>{}(d));
}
}

size_t compute(arg::shader_argument_shape const& v) { return detail::hash(v.num_srvs, v.num_uavs, v.has_cb); }

size_t compute(arg::shader_argument_shapes const v)
{
    size_t res = 0;
    for (auto const& e : v)
        res = detail::hash_combine(res, compute(e));
    return res;
}

size_t compute(arg::framebuffer_format const v)
{
    size_t res = 0;
    for (auto f_rt : v.render_targets)
        res = detail::hash_combine(res, detail::hash(f_rt));
    for (auto f_ds : v.depth_target)
        res = detail::hash_combine(res, detail::hash(f_ds));
    return res;
}

struct compute_functor
{
    template <class KeyT>
    std::size_t operator()(KeyT&& v) const { return compute(cc::forward<KeyT&&>(v)); }
};

}