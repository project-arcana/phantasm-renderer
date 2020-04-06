#pragma once

#include <cstdint>

namespace phi
{
struct backend_config;
struct window_handle;
class Backend;
}

namespace pr
{
enum class backend_type
{
    d3d12,
    vulkan
};

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

// shaders and pipeline states
struct shader_binary_pod;
struct graphics_pipeline_state_data;
struct compute_pipeline_state_data;
using shader_binary = auto_destroyer<shader_binary_pod, false>;
using graphics_pipeline_state = auto_destroyer<graphics_pipeline_state_data, false>;
using compute_pipeline_state = auto_destroyer<compute_pipeline_state_data, false>;

// shader arguments
struct argument;
struct prebuilt_argument_data;
using prebuilt_argument = auto_destroyer<prebuilt_argument_data, false>;

// RAII frame chain
namespace raii
{
class Frame;
class Framebuffer;
class GraphicsPass;
class ComputePass;
}

class CompiledFrame;
}
