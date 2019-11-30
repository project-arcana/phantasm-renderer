#pragma once

#include <clean-core/span.hh>

#include "types.hh"

namespace pr::backend::arg
{
struct framebuffer_format
{
    /// formats of the render targets, [0, n]
    cc::span<format const> render_targets;
    /// format of the depth stencil target, [0, 1]
    cc::span<format const> depth_target;

    constexpr bool operator==(framebuffer_format const& rhs) const noexcept
    {
        if (render_targets.size() != rhs.render_targets.size() || depth_target.size() != rhs.depth_target.size())
            return false;

        for (auto i = 0u; i < render_targets.size(); ++i)
            if (render_targets[i] != rhs.render_targets[i])
                return false;

        if (!depth_target.empty())
            if (depth_target[0] != rhs.depth_target[0])
                return false;

        return true;
    }
};

struct vertex_format
{
    cc::span<vertex_attribute_info const> attributes;
    unsigned vertex_size_bytes;
};

/// A shader argument consists of SRVs, UAVs, an optional CBV, and an offset into it
struct shader_argument_shape
{
    unsigned num_srvs = 0;
    unsigned num_uavs = 0;
    bool has_cb = false;

    constexpr bool operator==(shader_argument_shape const& rhs) const noexcept
    {
        return num_srvs == rhs.num_srvs && num_uavs == rhs.num_uavs && has_cb == rhs.has_cb;
    }
};

/// A shader payload consists of [1, 4] shader arguments
using shader_argument_shapes = cc::span<shader_argument_shape const>;

/// A shader stage
struct shader_stage
{
    /// pointer to the (backend-dependent) shader binary data
    std::byte* binary_data;
    size_t binary_size;
    /// the shader domain of this stage
    shader_domain domain;
};

/// A shader bundle consists of up to 1 stage per domain
using shader_stages = cc::span<shader_stage const>;


struct shader_view_element
{
    handle::resource resource;
    format pixel_format;
    shader_view_dimension dimension;

    struct sve_texture_info
    {
        unsigned mip_start;   ///< index of the first usable mipmap (usually: 0)
        unsigned mip_size;    ///< amount of usable mipmaps, starting from mip_start (usually: -1 / all)
        unsigned array_start; ///< index of the first usable array element [if applicable] (usually: 0)
        unsigned array_size;  ///< amount of usable array elements [if applicable]
    };

    struct sve_buffer_info
    {
        unsigned element_start;        ///< index of the first element in the buffer
        unsigned element_size;         ///< amount of elements in the buffer
        unsigned element_stride_bytes; ///< the stride of elements in bytes
    };

    union {
        sve_texture_info texture_info;
        sve_buffer_info buffer_info;
    };

    void init_as_tex2d(handle::resource res, format pf, unsigned mip_start = 0, unsigned mip_size = unsigned(-1))
    {
        resource = res;
        pixel_format = pf;
        dimension = shader_view_dimension::texture2d;
        texture_info.mip_start = mip_start;
        texture_info.mip_size = mip_size;
        texture_info.array_start = 0;
        texture_info.array_size = 1;
    }
};


inline bool operator==(shader_argument_shapes const& lhs, shader_argument_shapes const& rhs) noexcept
{
    if (lhs.size() != rhs.size())
        return false;

    for (auto i = 0u; i < lhs.size(); ++i)
    {
        if (!(lhs[i] == rhs[i]))
            return false;
    }

    return true;
}
}
