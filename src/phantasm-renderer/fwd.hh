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

// handle wrappers
template <class T>
struct raii_handle;
template <class T>
struct cached_handle;

// resources
struct resource_data;
using resource = raii_handle<resource_data>;

struct buffer;
struct image;
struct render_target;

using cached_buffer = cached_handle<buffer>;
using cached_target = cached_handle<render_target>;

// info argument objects
struct graphics_pass_info;
struct compute_pass_info;
struct framebuffer_info;

// shaders and pipeline states
struct shader_binary_data;
struct graphics_pipeline_state_data;
struct compute_pipeline_state_data;
using shader_binary = raii_handle<shader_binary_data>;
using graphics_pipeline_state = raii_handle<graphics_pipeline_state_data>;
using compute_pipeline_state = raii_handle<compute_pipeline_state_data>;

// shader arguments
struct argument;
struct prebuilt_argument_data;
using prebuilt_argument = raii_handle<prebuilt_argument_data>;

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
