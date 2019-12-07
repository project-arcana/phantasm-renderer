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
    unsigned num_samplers = 0;
    bool has_cb = false;

    constexpr bool operator==(shader_argument_shape const& rhs) const noexcept
    {
        return num_srvs == rhs.num_srvs && num_uavs == rhs.num_uavs && has_cb == rhs.has_cb && num_samplers == rhs.num_samplers;
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

// inline bool operator==(shader_sampler_configs const& lhs, shader_sampler_configs const& rhs) noexcept
//{
//    if (lhs.size() != rhs.size())
//        return false;

//    for (auto i = 0u; i < lhs.size(); ++i)
//    {
//        if (!(lhs[i] == rhs[i]))
//            return false;
//    }

//    return true;
//}
}
