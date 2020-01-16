#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include "limits.hh"
#include "types.hh"

namespace pr::backend::arg
{
struct framebuffer_config
{
    /// configs of the render targets, [0, n]
    cc::capped_vector<render_target_config, limits::max_render_targets> render_targets;

    bool logic_op_enable = false;
    blend_logic_op logic_op = blend_logic_op::no_op;

    /// format of the depth stencil target, [0, 1]
    cc::capped_vector<format, 1> depth_target;

    void add_render_target(format fmt)
    {
        render_target_config new_rt;
        new_rt.format = fmt;
        render_targets.push_back(new_rt);
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

struct blas_element
{
    handle::resource vertex_buffer = handle::null_resource;
    handle::resource index_buffer = handle::null_resource;
    unsigned num_vertices = 0;
    unsigned vertex_offset = 0; ///< offset in number of vertices
    unsigned num_indices = 0;
    unsigned index_offset = 0; ///< offset in number of indices
    bool is_opaque = true;
};
}
