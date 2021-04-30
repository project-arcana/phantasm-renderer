#pragma once

#include <clean-core/capped_vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/common/hashable_storage.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/common/api.hh>
#include <phantasm-renderer/common/state_info.hh>
#include <phantasm-renderer/enums.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/reflection/vertex_attributes.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
struct PR_API graphics_pass_info
{
public:
    /// Add a shader argument specifiying the amount of elements
    graphics_pass_info& arg(unsigned num_srvs, unsigned num_uavs = 0, unsigned num_samplers = 0, bool has_cbv = false)
    {
        _storage.get().arg_shapes.push_back({num_srvs, num_uavs, num_samplers, has_cbv});
        return *this;
    }

    /// Add a shader argument shape
    graphics_pass_info& arg(phi::arg::shader_arg_shape const& arg_shape)
    {
        _storage.get().arg_shapes.push_back(arg_shape);
        return *this;
    }

    /// add a shader argument matching the shape of a given pr::argument
    graphics_pass_info& arg(argument const& argument, bool has_cbv = false)
    {
        return arg(argument.get_num_srvs(), argument.get_num_uavs(), argument.get_num_samplers(), has_cbv);
    }

    /// Add root constants
    graphics_pass_info& enable_constants()
    {
        _storage.get().has_root_consts = true;
        return *this;
    }

    /// Set vertex size and attributes from data
    graphics_pass_info& vertex(uint32_t size_bytes, cc::span<phi::vertex_attribute_info const> attributes)
    {
        _storage.get().vertex_size_bytes = size_bytes;

        auto& attrs = _storage.get().vertex_attributes;
        attrs.clear();

        for (auto const& attr : attributes)
            attrs.push_back(attr);

        return *this;
    }

    /// Set vertex size and attributes from reflection of VertT
    template <class VertT>
    graphics_pass_info& vertex()
    {
        auto const attr = get_vertex_attributes<VertT>();
        return vertex(sizeof(VertT), attr);
    }

    /// Add a shader
    graphics_pass_info& shader(pr::shader_binary const& binary)
    {
        _storage.get().shader_hashes.push_back(binary._hash);
        _shaders.push_back(phi::arg::graphics_shader{{binary._data, binary._size}, binary._stage});
        return *this;
    }

    /// Set the pipeline config
    graphics_pass_info& config(phi::pipeline_config const& config)
    {
        _storage.get().graphics_config = config;
        return *this;
    }

    graphics_pass_info& depth_func(phi::depth_function func)
    {
        _storage.get().graphics_config.depth = func;
        return *this;
    }

    graphics_pass_info& depth_readonly(bool readonly = true)
    {
        _storage.get().graphics_config.depth_readonly = readonly;
        return *this;
    }

    graphics_pass_info& cull_mode(phi::cull_mode mode)
    {
        _storage.get().graphics_config.cull = mode;
        return *this;
    }

    graphics_pass_info& multisamples(int samples)
    {
        _storage.get().graphics_config.samples = samples;
        return *this;
    }

    graphics_pass_info& wireframe(bool enable = true)
    {
        _storage.get().graphics_config.wireframe = enable;
        return *this;
    }

    graphics_pass_info& frontface_winding(bool counterclockwise)
    {
        _storage.get().graphics_config.frontface_counterclockwise = counterclockwise;
        return *this;
    }

    graphics_pass_info& topology(pr::primitive_topology topology)
    {
        _storage.get().graphics_config.topology = topology;
        return *this;
    }

    uint64_t get_hash() const { return _storage.get_xxhash(); }

    hashable_storage<graphics_pass_info_data> _storage;
    cc::capped_vector<phi::arg::graphics_shader, 5> _shaders;
};

struct PR_API compute_pass_info
{
public:
    /// Add a shader argument specifiying the amount of elements
    compute_pass_info& arg(uint32_t num_srvs, uint32_t num_uavs = 0, uint32_t num_samplers = 0, bool has_cbv = false)
    {
        _storage.get().arg_shapes.push_back({num_srvs, num_uavs, num_samplers, has_cbv});
        return *this;
    }

    /// Add a shader argument shape
    compute_pass_info& arg(phi::arg::shader_arg_shape const& arg_shape)
    {
        _storage.get().arg_shapes.push_back(arg_shape);
        return *this;
    }

    /// add a shader argument matching the shape of a given pr::argument
    compute_pass_info& arg(argument const& argument, bool has_cbv = false)
    {
        return arg(argument.get_num_srvs(), argument.get_num_uavs(), argument.get_num_samplers(), has_cbv);
    }

    /// Add root constants
    compute_pass_info& enable_constants()
    {
        _storage.get().has_root_consts = true;
        return *this;
    }

    /// Add a shader
    compute_pass_info& shader(pr::shader_binary const& binary)
    {
        _storage.get().shader_hash = binary._hash;
        _shader = phi::arg::shader_binary{binary._data, binary._size};
        return *this;
    }

    [[nodiscard]] uint64_t get_hash() const { return _storage.get_xxhash(); }

    hashable_storage<compute_pass_info_data> _storage;
    phi::arg::shader_binary _shader;
};

template <class VertT, class... ShaderTs>
[[nodiscard]] graphics_pass_info graphics_pass(phi::pipeline_config const& config, ShaderTs const&... shaders)
{
    graphics_pass_info res;
    res.config(config);
    res.vertex<VertT>();
    (res.shader(shaders), ...);
    return res;
}

template <class... ShaderTs>
[[nodiscard]] graphics_pass_info graphics_pass(phi::pipeline_config const& config, ShaderTs const&... shaders)
{
    graphics_pass_info res;
    res.config(config);
    (res.shader(shaders), ...);
    return res;
}

template <class... ShaderTs>
[[nodiscard]] graphics_pass_info graphics_pass(ShaderTs const&... shaders)
{
    graphics_pass_info res;
    (res.shader(shaders), ...);
    return res;
}

template <class... ShaderTs>
[[nodiscard]] compute_pass_info compute_pass(ShaderTs const&... shaders)
{
    compute_pass_info res;
    (res.shader(shaders), ...);
    return res;
}

struct PR_API framebuffer_info
{
public:
    /// Add a render target based on format only
    framebuffer_info& target(pr::format format)
    {
        _storage.get().add_render_target(format);
        return *this;
    }

    /// Add a render target with blend state
    framebuffer_info& target(pr::format format, pr::blend_state blend_state)
    {
        _storage.get().render_targets.push_back(phi::render_target_config{format, true, blend_state});
        return *this;
    }

    /// Add a render target with blend configuration
    [[deprecated("Use target(format, blend_state) overload")]] framebuffer_info& target(phi::render_target_config config)
    {
        _storage.get().render_targets.push_back(config);
        return *this;
    }

    /// Set and enable the blend logic operation
    framebuffer_info& logic_op(phi::blend_logic_op op)
    {
        _storage.get().logic_op = op;
        _storage.get().logic_op_enable = true;
        return *this;
    }

    /// Add a depth render target based on format only
    framebuffer_info& depth(pr::format format)
    {
        _storage.get().set_depth_target(format);
        return *this;
    }

    uint64_t get_hash() const { return _storage.get_xxhash(); }

    hashable_storage<phi::arg::framebuffer_config> _storage;
};

template <class... FormatTs>
[[nodiscard]] framebuffer_info framebuffer(FormatTs... rt_formats)
{
    framebuffer_info res;
    (res.target(rt_formats), ...);
    return res;
}
}
