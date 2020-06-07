#pragma once

#include <cstdint>

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

template <class T, bool Cache>
struct auto_destroyer;

// resources
struct raw_resource;
struct buffer;
struct render_target;
struct texture;

using auto_buffer = auto_destroyer<buffer, false>;
using auto_render_target = auto_destroyer<render_target, false>;
using auto_texture = auto_destroyer<texture, false>;

using cached_buffer = auto_destroyer<buffer, true>;
using cached_render_target = auto_destroyer<render_target, true>;
using cached_texture = auto_destroyer<texture, true>;


// info argument objects
struct graphics_pass_info;
struct compute_pass_info;
struct framebuffer_info;

// shaders, PSOs, fences, query ranges
struct shader_binary;
struct pipeline_state_abstract;
struct graphics_pipeline_state;
struct compute_pipeline_state;
struct fence;
struct query_range;
using auto_shader_binary = auto_destroyer<shader_binary, false>;
using auto_graphics_pipeline_state = auto_destroyer<graphics_pipeline_state, false>;
using auto_compute_pipeline_state = auto_destroyer<compute_pipeline_state, false>;
using auto_fence = auto_destroyer<fence, false>;
using auto_query_range = auto_destroyer<query_range, false>;

// shader arguments
struct argument;
struct prebuilt_argument;
using auto_prebuilt_argument = auto_destroyer<prebuilt_argument, false>;

// RAII frame chain
namespace raii
{
class Frame;
class Framebuffer;
class GraphicsPass;
class ComputePass;
}

class CompiledFrame;

namespace detail
{
struct auto_destroy_proxy;
}
}
