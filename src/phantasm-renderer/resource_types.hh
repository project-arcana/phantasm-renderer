#pragma once

#include <clean-core/move.hh>
#include <clean-core/typedefs.hh>

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/resource_info.hh>

struct IDxcBlob;

namespace pr
{
class Context;

// ---- resource types ----

template <class HandleT>
struct raii_handle
{
    HandleT data;
    pr::Context* parent = nullptr;

    raii_handle() = default;
    raii_handle(HandleT const& data, Context* parent) : data(data), parent(parent) {}

    raii_handle(raii_handle&& rhs) noexcept : data(rhs.data), parent(rhs.parent) { rhs.parent = nullptr; }
    raii_handle& operator=(raii_handle&& rhs) noexcept
    {
        if (this != &rhs)
        {
            _destroy();
            data = rhs.data;
            parent = rhs.parent;
            rhs.parent = nullptr;
        }
        return *this;
    }

    raii_handle(raii_handle const&) = delete;
    raii_handle& operator=(raii_handle const&) = delete;

    ~raii_handle() { _destroy(); }

private:
    void _destroy()
    {
        if (parent != nullptr)
        {
            data.destroy(parent);
            parent = nullptr;
        }
    }
};

template <class CachedT>
struct cached_handle
{
    operator CachedT&() { return _internal; }
    operator CachedT const&() const { return _internal; }

    cached_handle() = default;
    cached_handle(CachedT&& data, Context* parent) : _internal(cc::move(data)), _parent(parent) {}

    cached_handle(cached_handle&& rhs) noexcept : _internal(cc::move(rhs._internal)), _parent(rhs._parent) { rhs._parent = nullptr; }
    cached_handle& operator=(cached_handle&& rhs) noexcept
    {
        if (this != &rhs)
        {
            _destroy();
            _internal = cc::move(rhs._internal);
            _parent = rhs._parent;
            rhs.parent = nullptr;
        }
        return *this;
    }

    cached_handle(cached_handle const&) = delete;
    cached_handle& operator=(cached_handle const&) = delete;

    ~cached_handle() { _destroy(); }

private:
    void _destroy()
    {
        if (_parent != nullptr)
        {
            _internal.unrefCache(_parent);
            _parent = nullptr;
        }
    }

    CachedT _internal;
    pr::Context* _parent = nullptr;
};

struct resource_data
{
    phi::handle::resource handle = phi::handle::null_resource;
    uint64_t guid = 0; // for shader_view caching

    void destroy(pr::Context* ctx);
};

using resource = raii_handle<resource_data>;

// buffers

struct buffer
{
    resource _resource;
    buffer_info _info;

    void unrefCache(pr::Context* ctx);
};

using cached_buffer = cached_handle<buffer>;
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

    void unrefCache(pr::Context* ctx);
};

using cached_render_target = cached_handle<render_target>;

// shaders

struct shader_binary_data
{
    std::byte const* _data = nullptr;
    size_t _size = 0;
    IDxcBlob* _owning_blob = nullptr; ///< if non-null, shader was compiled online and must be freed via dxc
    uint64_t _guid = 0;               ///< for PSO caching
    Context* _parent = nullptr;
    phi::shader_stage _stage;

    void destroy(pr::Context* ctx);
};

using shader_binary = raii_handle<shader_binary_data>;

struct shader_binary_unreffable : public shader_binary
{
    void unrefCache(pr::Context* ctx);
};

using cached_shader_binary = cached_handle<shader_binary_unreffable>;

// PSOs

struct pipeline_state_abstract // cached internally, 1:1, based on all arguments to a pso creation + shader data hash
{
    phi::handle::pipeline_state _handle = phi::handle::null_pipeline_state;
    void destroy(pr::Context* ctx);
};

struct graphics_pipeline_state_data : public pipeline_state_abstract
{
};
struct compute_pipeline_state_data : public pipeline_state_abstract
{
};

using graphics_pipeline_state = raii_handle<graphics_pipeline_state_data>;
using compute_pipeline_state = raii_handle<compute_pipeline_state_data>;

};
