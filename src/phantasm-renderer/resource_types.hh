#pragma once

#include <clean-core/move.hh>
#include <clean-core/typedefs.hh>

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/murmur_hash.hh>
#include <phantasm-renderer/common/resource_info.hh>

struct IDxcBlob;

namespace pr
{
class Context;

// ---- resource types ----

template <class T, bool Cache>
struct auto_destroyer
{
    T data;

    auto_destroyer() = default;
    auto_destroyer(T const& data, pr::Context* parent) : data(data), parent(parent) {}

    auto_destroyer(auto_destroyer&& rhs) noexcept : data(rhs.data), parent(rhs.parent) { rhs.parent = nullptr; }
    auto_destroyer& operator=(auto_destroyer&& rhs) noexcept
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

    auto_destroyer(auto_destroyer const&) = delete;
    auto_destroyer& operator=(auto_destroyer const&) = delete;

    ~auto_destroyer() { _destroy(); }

    /// disable RAII management and receive the POD contents, which must now be manually freed
    T const& unlock()
    {
        parent = nullptr;
        return data;
    }

    operator T&() & { return data; }
    operator T const&() const& { return data; }

    // force unlock from rvalue
    operator T&() && = delete;
    operator T const&() const&& = delete;

private:
    pr::Context* parent = nullptr;

    void _destroy()
    {
        if (parent != nullptr)
        {
            if constexpr (Cache)
            {
                data.cache_deref(parent);
            }
            else
            {
                data.destroy(parent);
            }

            parent = nullptr;
        }
    }
};

struct raw_resource
{
    phi::handle::resource handle;
    uint64_t guid;
};

struct buffer
{
    raw_resource res;
    buffer_info info;

private:
    friend auto_destroyer<buffer, true>;
    friend auto_destroyer<buffer, false>;
    void cache_deref(pr::Context* parent);
    void destroy(pr::Context* parent);
};

struct render_target
{
    raw_resource res;
    render_target_info info;

private:
    friend auto_destroyer<render_target, true>;
    friend auto_destroyer<render_target, false>;
    void cache_deref(pr::Context* parent);
    void destroy(pr::Context* parent);
};

struct texture
{
    raw_resource res;
    texture_info info;

private:
    friend auto_destroyer<texture, true>;
    friend auto_destroyer<texture, false>;
    void cache_deref(pr::Context* parent);
    void destroy(pr::Context* parent);
};

// move-only lifetime management
using auto_buffer = auto_destroyer<buffer, false>;
using auto_render_target = auto_destroyer<render_target, false>;
using auto_texture = auto_destroyer<texture, false>;

using cached_buffer = auto_destroyer<buffer, true>;
using cached_render_target = auto_destroyer<render_target, true>;
using cached_texture = auto_destroyer<texture, true>;

// shaders

struct shader_binary_pod
{
    std::byte const* _data = nullptr;
    size_t _size = 0;
    IDxcBlob* _owning_blob = nullptr; ///< if non-null, shader was compiled online and must be freed via dxc
    murmur_hash _hash;                ///< murmur hash over _data, for caching of PSOs using this shader
    phi::shader_stage _stage;

private:
    friend struct auto_destroyer<shader_binary_pod, false>;
    void destroy(pr::Context* ctx);
};

using shader_binary = auto_destroyer<shader_binary_pod, false>;

// PSOs

struct pipeline_state_abstract
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

using graphics_pipeline_state = auto_destroyer<graphics_pipeline_state_data, false>;
using compute_pipeline_state = auto_destroyer<compute_pipeline_state_data, false>;

};
