#pragma once

#include <clean-core/typedefs.hh>

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/resource_info.hh>

namespace pr
{
// ---- resource types ----

struct resource
{
    phi::handle::resource _handle = phi::handle::null_resource;
    uint64_t _guid; // for shader_view caching
};

// buffers

struct buffer
{
    resource _resource;
    buffer_info _info;
};

struct cached_buffer // cached 1:N
{
    operator buffer&() { return _internal; }
    operator buffer const&() const { return _internal; }

    buffer _internal;
};

// textures

struct image
{
    resource _resource;
    texture_info _info;
};

struct render_target
{
    resource _resource;
    render_target_info _info;
};

struct cached_render_target // cached 1:N
{
    operator render_target&() { return _internal; }
    operator render_target const&() const { return _internal; }

    render_target _internal;
};

// shaders

struct shader_binary
{
    std::byte* _data;
    size_t _size;
    phi::shader_stage _stage;
    cc::hash_t _data_hash; // murmur over binary for PSO caching
};

struct cached_shader_binary // cached 1:1, from murmur over text (HLSL)
{
    operator shader_binary&() { return _internal; }
    operator shader_binary const&() const { return _internal; }

    shader_binary _internal;
};

// PSOs

struct pipeline_state_abstract // cached internally, 1:1, based on all arguments to a pso creation + shader data hash
{
    phi::handle::pipeline_state _handle = phi::handle::null_pipeline_state;
};

struct graphics_pipeline_state : public pipeline_state_abstract
{
};
struct compute_pipeline_state : public pipeline_state_abstract
{
};

// Shader views

struct shader_view // cached internally, 1:1, based on guids, element types, sampler configs
{
    phi::handle::shader_view _handle = phi::handle::null_shader_view;
    int num_srvs, num_uavs, num_samplers;
};

};
