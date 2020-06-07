#pragma once

#include <phantasm-hardware-interface/types.hh>

namespace pr
{
enum class backend
{
    d3d12,
    vulkan
};

// enum renames
using shader = phi::shader_stage;
using format = phi::format;
using state = phi::resource_state;

// enum passthroughs
using phi::blend_factor;
using phi::blend_logic_op;
using phi::blend_op;
using phi::cull_mode;
using phi::depth_function;
using phi::primitive_topology;
using phi::query_type;
using phi::queue_type;
using phi::resource_heap;
using phi::sampler_address_mode;
using phi::sampler_border_color;
using phi::sampler_compare_func;
using phi::sampler_filter;

// type renames
using shader_flags = phi::shader_stage_flags_t;
using clear_value = phi::rt_clear_value;

// type passthroughs
using phi::blend_state;
using phi::pipeline_config;
using phi::sampler_config;
}
