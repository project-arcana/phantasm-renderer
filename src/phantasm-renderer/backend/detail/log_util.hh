#pragma once

#include <phantasm-renderer/backend/command_stream.hh>

namespace pr::backend::log
{
void dump_hex(char const* desc, void const* addr, int len);
}
