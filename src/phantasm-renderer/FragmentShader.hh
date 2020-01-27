#pragma once

#include <phantasm-hardware-interface/arguments.hh>

#include "format.hh"

namespace pr
{
template <format FragmentF>
class FragmentShader
{
public:
    phi::arg::shader_binary binary;
};
}
