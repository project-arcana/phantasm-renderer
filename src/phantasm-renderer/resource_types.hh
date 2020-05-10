#pragma once

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/enums.hh>
#include <phantasm-renderer/fwd.hh>

#include <phantasm-renderer/common/murmur_hash.hh>
#include <phantasm-renderer/common/resource_info.hh>

#include <phantasm-renderer/detail/auto_destroyer.hh>

#include <typed-geometry/tg-lean.hh>

namespace pr
{
//
// resource types

struct raw_resource
{
    phi::handle::resource handle;
    uint64_t guid;
};

struct buffer
{
    raw_resource res;
    buffer_info info;
};

struct render_target
{
    raw_resource res;
    render_target_info info;

    int samples() const { return info.num_samples; }
    tg::isize2 size() const { return {info.width, info.height}; }
    pr::format format() const { return info.format; }
};

struct texture
{
    raw_resource res;
    texture_info info;
};

//
// shaders

struct shader_binary
{
    std::byte const* _data = nullptr;
    size_t _size = 0;
    IDxcBlob* _owning_blob = nullptr; ///< if non-null, shader was compiled online and must be freed via dxc
    murmur_hash _hash;                ///< murmur hash over _data, for caching of PSOs using this shader
    phi::shader_stage _stage;
};

//
// PSOs

struct pipeline_state_abstract
{
    phi::handle::pipeline_state _handle = phi::handle::null_pipeline_state;
};

struct graphics_pipeline_state : public pipeline_state_abstract
{
};
struct compute_pipeline_state : public pipeline_state_abstract
{
};

//
// auto_ and cached_ aliases

// move-only, self-destructing versions
using auto_buffer = auto_destroyer<buffer, false>;
using auto_render_target = auto_destroyer<render_target, false>;
using auto_texture = auto_destroyer<texture, false>;

using auto_shader_binary = auto_destroyer<shader_binary, false>;
using auto_graphics_pipeline_state = auto_destroyer<graphics_pipeline_state, false>;
using auto_compute_pipeline_state = auto_destroyer<compute_pipeline_state, false>;

// move-only, self-cachefreeing versions
using cached_buffer = auto_destroyer<buffer, true>;
using cached_render_target = auto_destroyer<render_target, true>;
using cached_texture = auto_destroyer<texture, true>;


};
