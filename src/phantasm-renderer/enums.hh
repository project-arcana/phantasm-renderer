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
}
