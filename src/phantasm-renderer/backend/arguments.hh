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

struct shader_binary
{
    std::byte* data; ///< pointer to the (backend-dependent) shader binary data
    size_t size;
};

struct shader_stage
{
    shader_binary binary;
    shader_domain domain;
};

/// A graphics shader bundle consists of up to 1 shader per graphics stage
using graphics_shader_stages = cc::span<shader_stage const>;

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
    handle::resource index_buffer = handle::null_resource; ///< can be null
    unsigned num_vertices = 0;
    unsigned num_indices = 0;
    bool is_opaque = true;
};

struct raytracing_shader_library
{
    shader_binary binary;
    cc::span<wchar_t const* const> symbols;
    cc::capped_vector<shader_argument_shape, limits::max_shader_arguments> argument_shapes;
    bool has_root_constants = false;
};

using raytracing_shader_libraries = cc::span<raytracing_shader_library const>;

struct raytracing_hit_group
{
    wchar_t const* name;
    wchar_t const* closest_hit_symbol;
    wchar_t const* any_hit_symbol = L"";
    wchar_t const* intersection_symbol = L"";
};

using raytracing_hit_groups = cc::span<raytracing_hit_group const>;

}
