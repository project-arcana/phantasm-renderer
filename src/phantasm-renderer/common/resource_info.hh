#pragma once

#include <clean-core/hash.hh>

#include <phantasm-hardware-interface/types.hh>

namespace pr
{
struct render_target_info
{
    phi::format format;
    int width;
    int height;
    unsigned num_samples;

    constexpr bool operator==(render_target_info const& rhs) const noexcept
    {
        return format == rhs.format && width == rhs.width && height == rhs.height && num_samples == rhs.num_samples;
    }
};

struct texture_info
{
    phi::format format;
    phi::texture_dimension dim;
    bool allow_uav;
    int width;
    int height;
    unsigned depth_or_array_size;
    unsigned num_mips;

    constexpr bool operator==(texture_info const& rhs) const noexcept
    {
        return format == rhs.format && dim == rhs.dim && allow_uav == rhs.allow_uav && width == rhs.width && height == rhs.height
               && depth_or_array_size == rhs.depth_or_array_size && num_mips == rhs.num_mips;
    }
};

struct buffer_info
{
    unsigned size_bytes;
    unsigned stride_bytes;
    bool allow_uav;
    bool is_mapped;

    constexpr bool operator==(buffer_info const& rhs) const noexcept
    {
        return size_bytes == rhs.size_bytes && stride_bytes == rhs.stride_bytes && allow_uav == rhs.allow_uav && is_mapped == rhs.is_mapped;
    }
};

struct resource_info_hasher
{
    constexpr size_t operator()(render_target_info const& rt) const noexcept { return cc::make_hash(rt.format, rt.width, rt.height, rt.num_samples); }

    constexpr size_t operator()(texture_info const& t) const noexcept
    {
        return cc::make_hash(t.format, t.dim, t.allow_uav, t.width, t.height, t.depth_or_array_size, t.num_mips);
    }

    constexpr size_t operator()(buffer_info const& b) const noexcept { return cc::make_hash(b.size_bytes, b.stride_bytes, b.allow_uav, b.is_mapped); }
};
}
