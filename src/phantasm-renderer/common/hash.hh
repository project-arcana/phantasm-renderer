#pragma once

#include <clean-core/hash.hh>
#include <clean-core/span.hh>

#include <phantasm-hardware-interface/types.hh>

namespace pr
{
namespace hash
{
inline cc::hash_t compute(phi::resource_view const& rv)
{
    auto res = cc::make_hash(rv.resource.index, rv.pixel_format, rv.dimension);

    if (rv.dimension == phi::resource_view_dimension::buffer)
    {
        res = cc::combine_hash(res, cc::make_hash(rv.buffer_info.num_elements, rv.buffer_info.element_start, rv.buffer_info.element_stride_bytes));
    }
    else if (rv.dimension != phi::resource_view_dimension::raytracing_accel_struct)
    {
        res = cc::combine_hash(res, cc::make_hash(rv.texture_info.mip_start, rv.texture_info.mip_size, rv.texture_info.array_start, rv.texture_info.array_size));
    }

    return res;
}

inline cc::hash_t compute(phi::sampler_config const& sc)
{
    return cc::make_hash(sc.filter, sc.address_u, sc.address_v, sc.address_w, sc.min_lod, sc.max_lod, sc.lod_bias, sc.max_anisotropy, sc.compare_func,
                         sc.border_color);
}

inline cc::hash_t compute(cc::span<phi::resource_view const> srvs,
                          cc::span<phi::resource_view const> uavs,
                          cc::span<uint64_t const> guids,
                          cc::span<phi::sampler_config const> samplers)
{
    cc::hash_t res = 0;

    for (auto const& srv : srvs)
        res = cc::hash_combine(res, compute(srv));

    for (auto const& uav : uavs)
        res = cc::hash_combine(res, compute(uav));

    for (auto const& guid : guids)
        res = cc::hash_combine(res, compute(guid));

    for (auto const& sc : samplers)
        res = cc::hash_combine(res, compute(sc));

    return res;
}


}
}
