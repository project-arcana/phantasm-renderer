#pragma once

#include <clean-core/capped_vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/common/hashable_storage.hh>

#include <phantasm-renderer/common/state_info.hh>
#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/reflection/vertex_attributes.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
class Context;
class Frame;

struct graphics_pass_info
{
public:
    /// Add a shader argument specifiying the amount of elements
    graphics_pass_info& arg(unsigned num_srvs, unsigned num_uavs = 0, unsigned num_samplers = 0, bool has_cbv = false)
    {
        _storage.get().arg_shapes.push_back({num_srvs, num_uavs, num_samplers, has_cbv});
        return *this;
    }

    /// Add root constants
    graphics_pass_info& constants()
    {
        _storage.get().has_root_consts = true;
        return *this;
    }

    /// Set vertex size and attributes from data
    graphics_pass_info& vertex(unsigned size_bytes, cc::span<phi::vertex_attribute_info const> attributes)
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
        return vertex(sizeof(VertT), get_vertex_attributes<VertT>());
    }

    /// Add a shader
    graphics_pass_info& shader(pr::shader_binary const& binary)
    {
        _storage.get().shader_hashes.push_back(binary.data._hash);
        _shaders.push_back(phi::arg::graphics_shader{{binary.data._data, binary.data._size}, binary.data._stage});
        return *this;
    }

    /// Set the pipeline config
    graphics_pass_info& config(phi::pipeline_config const& config)
    {
        // this could also have finegrained setters for each option
        _storage.get().graphics_config = config;
        return *this;
    }

private:
    friend class raii::Frame;
    [[nodiscard]] murmur_hash get_hash() const
    {
        murmur_hash res;
        _storage.get_murmur(res);
        return res;
    }

private:
    friend class Context;
    hashable_storage<graphics_pass_info_data> _storage;
    cc::capped_vector<phi::arg::graphics_shader, 5> _shaders;
};

struct compute_pass_info
{
public:
    /// Add a shader argument specifiying the amount of elements
    compute_pass_info& arg(unsigned num_srvs, unsigned num_uavs = 0, unsigned num_samplers = 0, bool has_cbv = false)
    {
        _storage.get().arg_shapes.push_back({num_srvs, num_uavs, num_samplers, has_cbv});
        return *this;
    }

    /// Add root constants
    compute_pass_info& constants()
    {
        _storage.get().has_root_consts = true;
        return *this;
    }

    /// Add a shader
    compute_pass_info& shader(pr::shader_binary const& binary)
    {
        _storage.get().shader_hash = binary.data._hash;
        _shader = phi::arg::shader_binary{binary.data._data, binary.data._size};
        return *this;
    }

private:
    friend class Frame;
    [[nodiscard]] murmur_hash get_hash() const
    {
        murmur_hash res;
        _storage.get_murmur(res);
        return res;
    }

private:
    friend class Context;
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

struct framebuffer_info
{
public:
    /// Add a render target based on format only
    framebuffer_info& target(pr::format format)
    {
        _storage.get().add_render_target(format);
        return *this;
    }

    /// Add a render target with blend configuration
    framebuffer_info& target(phi::render_target_config config)
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
        _storage.get().add_depth_target(format);
        return *this;
    }

private:
    friend class raii::Frame;
    [[nodiscard]] murmur_hash get_hash() const
    {
        murmur_hash res;
        _storage.get_murmur(res);
        return res;
    }

private:
    friend class Context;
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
