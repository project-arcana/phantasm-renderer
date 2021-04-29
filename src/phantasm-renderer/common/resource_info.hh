#pragma once

#include <clean-core/hash.hh>

#include <phantasm-hardware-interface/arguments.hh>

namespace pr
{
using texture_info = phi::arg::texture_description;
using buffer_info = phi::arg::buffer_description;
using generic_resource_info = phi::arg::resource_description;

struct resource_info_hasher
{
    constexpr size_t operator()(texture_info const& t) const noexcept
    {
        return cc::make_hash(t.fmt, t.dim, t.usage, t.width, t.height, t.depth_or_array_size, t.num_mips, t.num_samples, t.optimized_clear_value);
    }

    constexpr size_t operator()(buffer_info const& b) const noexcept { return cc::make_hash(b.size_bytes, b.stride_bytes, b.allow_uav, b.heap); }
};
}
