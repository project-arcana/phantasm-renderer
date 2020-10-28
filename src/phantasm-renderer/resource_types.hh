#pragma once

#include <typed-geometry/types/size.hh>

#include <phantasm-hardware-interface/handles.hh>

#include <phantasm-renderer/common/resource_info.hh>
#include <phantasm-renderer/detail/auto_destroyer.hh>
#include <phantasm-renderer/enums.hh>


namespace pr
{
//
// resource types

struct raw_resource
{
    phi::handle::resource handle = phi::handle::null_resource;
    uint64_t guid = 0;
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
    cc::hash_t _hash;                 ///< xxhash64 over _data, for caching of PSOs using this shader
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
// auxilliary

struct fence
{
    phi::handle::fence handle = phi::handle::null_fence;
};

struct query_range
{
    phi::handle::query_range handle = phi::handle::null_query_range;
    pr::query_type type;
    unsigned num;
};

struct swapchain
{
    phi::handle::swapchain handle = phi::handle::null_swapchain;
};

//
// auto_ and cached_ aliases

// move-only, self-destructing versions
using auto_buffer = auto_destroyer<buffer, auto_mode::guard>;
using auto_render_target = auto_destroyer<render_target, auto_mode::guard>;
using auto_texture = auto_destroyer<texture, auto_mode::guard>;

using auto_shader_binary = auto_destroyer<shader_binary, auto_mode::destroy>; // this is a CPU-only type, allow auto destruction
using auto_graphics_pipeline_state = auto_destroyer<graphics_pipeline_state, auto_mode::guard>;
using auto_compute_pipeline_state = auto_destroyer<compute_pipeline_state, auto_mode::guard>;
using auto_fence = auto_destroyer<fence, auto_mode::guard>;
using auto_query_range = auto_destroyer<query_range, auto_mode::guard>;
using auto_swapchain = auto_destroyer<swapchain, auto_mode::guard>;

// move-only, self-cachefreeing versions
using cached_buffer = auto_destroyer<buffer, auto_mode::cache>;
using cached_render_target = auto_destroyer<render_target, auto_mode::cache>;
using cached_texture = auto_destroyer<texture, auto_mode::cache>;


};
