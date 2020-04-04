#pragma once

#include <clean-core/capped_vector.hh>

#include <phantasm-hardware-interface/arguments.hh>

#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/reflection/vertex_attributes.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
struct graphics_pass_info
{
    phi::pipeline_config _graphics_config = {};
    unsigned _vertex_size_bytes = 0;
    bool _has_root_consts = false;
    cc::capped_vector<phi::vertex_attribute_info, 8> _vertex_attributes;
    cc::capped_vector<phi::arg::shader_arg_shape, phi::limits::max_shader_arguments> _arg_shapes;
    cc::capped_vector<phi::arg::graphics_shader, 5> _shaders;
};

struct pipeline_builder
{
public:
    pipeline_builder& add_shader(pr::shader_binary const& binary)
    {
        _shaders.push_back(phi::arg::graphics_shader{{binary.data._data, binary.data._size}, binary.data._stage});
        return *this;
    }

    pipeline_builder& add_root_constants()
    {
        _has_root_consts = true;
        return *this;
    }

    pipeline_builder& add_argument_shape(unsigned num_srvs, unsigned num_uavs = 0, unsigned num_samplers = 0, bool has_cbv = false)
    {
        _arg_shapes.push_back({num_srvs, num_uavs, num_samplers, has_cbv});
        return *this;
    }

    pipeline_builder& set_config(phi::pipeline_config const& config)
    {
        // this could also have finegrained setters for each option
        _graphics_config = config;
        return *this;
    }

    // vertex

    pipeline_builder& set_vertex_format(unsigned size_bytes, cc::span<phi::vertex_attribute_info const> attributes)
    {
        _vertex_size_bytes = size_bytes;
        _vertex_attributes = attributes;
        return *this;
    }

    template <class VertT>
    pipeline_builder& set_vertex_format()
    {
        _vertex_size_bytes = sizeof(VertT);
        auto const attrs = get_vertex_attributes<VertT>();
        _vertex_attributes.resize(attrs.size());
        cc::copy(attrs, _vertex_attributes);
        return *this;
    }

    // framebuffer

    pipeline_builder& add_render_target(pr::format format)
    {
        _framebuffer_config.add_render_target(format);
        return *this;
    }

    pipeline_builder& add_render_target(phi::render_target_config config)
    {
        _framebuffer_config.render_targets.push_back(config);
        return *this;
    }

    pipeline_builder& set_framebuffer_logic_op(phi::blend_logic_op op)
    {
        _framebuffer_config.logic_op = op;
        _framebuffer_config.logic_op_enable = true;
        return *this;
    }

    pipeline_builder& add_depth_target(pr::format format)
    {
        _framebuffer_config.add_depth_target(format);
        return *this;
    }

    // finalize

    [[nodiscard]] compute_pipeline_state make_compute();
    [[nodiscard]] graphics_pipeline_state make_graphics();

private:
    friend class Context;
    pipeline_builder(Context* parent) : _parent(parent) {}
    Context* const _parent;

    phi::pipeline_config _graphics_config = {};
    unsigned _vertex_size_bytes = 0;
    bool _has_root_consts = false;
    cc::capped_vector<phi::vertex_attribute_info, 8> _vertex_attributes;
    cc::capped_vector<phi::arg::shader_arg_shape, phi::limits::max_shader_arguments> _arg_shapes;
    cc::capped_vector<phi::arg::graphics_shader, 5> _shaders;
    phi::arg::framebuffer_config _framebuffer_config;
};

}
