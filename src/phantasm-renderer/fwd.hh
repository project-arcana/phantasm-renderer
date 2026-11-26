#pragma once

#include <stdint.h>

struct IDxcBlob;

namespace phi
{
struct backend_config;
struct window_handle;
class Backend;
}

namespace pr
{
class Context;
using gpu_epoch_t = uint64_t;

enum class auto_mode : uint8_t
{
    guard,   // assert that the resource was disowned
    destroy, // destroy the resource
    cache    // cache-dereference the resource
};

template <class T, auto_mode Cache>
struct auto_destroyer;

// resources
struct resource;
struct buffer;
struct texture;

using auto_buffer = auto_destroyer<buffer, auto_mode::guard>;
using auto_texture = auto_destroyer<texture, auto_mode::guard>;

using cached_buffer = auto_destroyer<buffer, auto_mode::cache>;
using cached_texture = auto_destroyer<texture, auto_mode::cache>;


// info argument objects
struct graphics_pass_info;
struct compute_pass_info;
struct framebuffer_info;
struct graphics_pass_info_data;
struct compute_pass_info_data;
struct freeable_cached_obj;

// shaders, PSOs, fences, query ranges
struct shader_binary;
struct pipeline_state_abstract;
struct graphics_pipeline_state;
struct compute_pipeline_state;
struct fence;
struct query_range;
struct swapchain;
using auto_shader_binary = auto_destroyer<shader_binary, auto_mode::destroy>;
using auto_graphics_pipeline_state = auto_destroyer<graphics_pipeline_state, auto_mode::guard>;
using auto_compute_pipeline_state = auto_destroyer<compute_pipeline_state, auto_mode::guard>;
using auto_fence = auto_destroyer<fence, auto_mode::guard>;
using auto_query_range = auto_destroyer<query_range, auto_mode::guard>;
using auto_swapchain = auto_destroyer<swapchain, auto_mode::guard>;

// shader arguments
struct view;
struct argument;
struct prebuilt_argument;
struct argument_builder;
using auto_prebuilt_argument = auto_destroyer<prebuilt_argument, auto_mode::guard>;

// RAII frame chain
namespace raii
{
class Frame;
class Framebuffer;
class GraphicsPass;
class ComputePass;
}

class CompiledFrame;
template <class T>
struct hashable_storage;

namespace detail
{
struct auto_destroy_proxy;
}
}
