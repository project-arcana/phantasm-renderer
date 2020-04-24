#pragma once

#include <phantasm-hardware-interface/types.hh>

namespace pr
{
enum class backend
{
    d3d12,
    vulkan
};

using shader = phi::shader_stage;
using format = phi::format;
using state = phi::resource_state;

using phi::blend_factor;
using phi::blend_logic_op;
using phi::blend_op;
using phi::sampler_address_mode;
using phi::sampler_border_color;
using phi::sampler_compare_func;
using phi::sampler_filter;
}
