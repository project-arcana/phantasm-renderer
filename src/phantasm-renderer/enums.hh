#pragma once

#include <phantasm-hardware-interface/arguments.hh>
#include <phantasm-hardware-interface/types.hh>

namespace pr
{
enum class backend
{
    d3d12,
    vulkan
};

// enum renames
using shader_stage = phi::shader_stage;
using shader = phi::shader_stage_flags;
using shader_flags = phi::shader_stage_flags_t;
using format = phi::format;
using state = phi::resource_state;

// enum passthroughs
using phi::blend_factor;
using phi::blend_logic_op;
using phi::blend_op;
using phi::cull_mode;
using phi::depth_function;
using phi::present_mode;
using phi::primitive_topology;
using phi::query_type;
using phi::queue_type;
using phi::resource_heap;
using phi::sampler_address_mode;
using phi::sampler_border_color;
using phi::sampler_compare_func;
using phi::sampler_filter;

// type renames
using clear_value = phi::rt_clear_value;

// type passthroughs
using blend_state = phi::arg::blend_state;
using phi::buffer_address;
using phi::buffer_range;
using pipeline_config = phi::arg::pipeline_config;
using phi::sampler_config;
}
