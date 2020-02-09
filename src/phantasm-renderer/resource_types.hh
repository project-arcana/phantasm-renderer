#pragma once

#include <clean-core/typedefs.hh>

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/resource_info.hh>

struct IDxcBlob;

namespace pr
{
class Context;

// ---- resource types ----

struct resource
{
    // move-only RAII

    phi::handle::resource _handle = phi::handle::null_resource;
    uint64_t _guid = 0; // for shader_view caching
    Context* _parent = nullptr;

    resource() = default;
    resource(phi::handle::resource handle, uint64_t guid, Context* parent) : _handle(handle), _guid(guid), _parent(parent) {}

    resource(resource&& rhs) noexcept : _handle(rhs._handle), _guid(rhs._guid), _parent(rhs._parent) { rhs._parent = nullptr; }
    resource& operator=(resource&& rhs) noexcept
    {
        if (this != &rhs)
        {
            _destroy();
            _handle = rhs._handle;
            _guid = rhs._guid;
            _parent = rhs._parent;
            rhs._parent = nullptr;
        }
        return *this;
    }

    resource(resource const&) = delete;
    resource& operator=(resource const&) = delete;

    ~resource() { _destroy(); }

private:
    void _destroy();
};

// buffers

struct buffer
{
    resource _resource;
    buffer_info _info;
};

struct cached_buffer // cached 1:N
{
    // move-only RAII

    operator buffer&() { return _internal; }
    operator buffer const&() const { return _internal; }

    buffer _internal;

    ~cached_buffer();
    cached_buffer(cached_buffer&&) noexcept = default;
    cached_buffer& operator=(cached_buffer&&) noexcept = default;
    cached_buffer(cached_buffer const&) = delete;
    cached_buffer& operator=(cached_buffer const&) = delete;
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
    // move-only RAII

    operator render_target&() { return _internal; }
    operator render_target const&() const { return _internal; }

    render_target _internal;

    ~cached_render_target();
    cached_render_target(cached_render_target&&) noexcept = default;
    cached_render_target& operator=(cached_render_target&&) noexcept = default;
    cached_render_target(cached_render_target const&) = delete;
    cached_render_target& operator=(cached_render_target const&) = delete;
};

// shaders

struct shader_binary
{
    std::byte const* _data = nullptr;
    size_t _size = 0;
    IDxcBlob* _owning_blob = nullptr; ///< if non-null, shader was compiled online and must be freed via dxc
    uint64_t _guid = 0;               ///< for PSO caching
    Context* _parent = nullptr;
    phi::shader_stage _stage;

    shader_binary() = default;

    shader_binary(shader_binary&& rhs) noexcept
      : _data(rhs._data), _size(rhs._size), _owning_blob(rhs._owning_blob), _guid(rhs._guid), _parent(rhs._parent), _stage(rhs._stage)
    {
        rhs._parent = nullptr;
    }

    shader_binary& operator=(shader_binary&& rhs) noexcept
    {
        if (this != &rhs)
        {
            _destroy();
            _data = rhs._data;
            _size = rhs._size;
            _owning_blob = rhs._owning_blob;
            _guid = rhs._guid;
            _parent = rhs._parent;
            _stage = rhs._stage;
            rhs._parent = nullptr;
        }
        return *this;
    }

    shader_binary(shader_binary const&) = delete;
    shader_binary& operator=(shader_binary const&) = delete;

    ~shader_binary() { _destroy(); }

private:
    void _destroy();
};

struct cached_shader_binary // cached 1:1, from murmur over text (HLSL)
{
    operator shader_binary&() { return _internal; }
    operator shader_binary const&() const { return _internal; }

    shader_binary _internal;
    // TODO dtor / behavior in general
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
