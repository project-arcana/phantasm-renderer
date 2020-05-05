#pragma once

#include <clean-core/hash.hh>

#include <phantasm-hardware-interface/arguments.hh>

namespace pr
{
using render_target_info = phi::arg::create_render_target_info;
using texture_info = phi::arg::create_texture_info;
using buffer_info = phi::arg::create_buffer_info;

struct resource_info_hasher
{
    constexpr size_t operator()(render_target_info const& rt) const noexcept
    {
        return cc::make_hash(rt.format, rt.width, rt.height, rt.num_samples, rt.array_size);
    }

    constexpr size_t operator()(texture_info const& t) const noexcept
    {
        return cc::make_hash(t.fmt, t.dim, t.allow_uav, t.width, t.height, t.depth_or_array_size, t.num_mips);
    }

    constexpr size_t operator()(buffer_info const& b) const noexcept { return cc::make_hash(b.size_bytes, b.stride_bytes, b.allow_uav, b.heap); }
};
}
