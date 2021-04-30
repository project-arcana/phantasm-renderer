#pragma once

#include <clean-core/fwd.hh>

#include <phantasm-hardware-interface/fwd.hh>

#include <phantasm-renderer/enums.hh>

namespace pr::detail
{
[[nodiscard]] phi::Backend* make_backend(backend type, cc::allocator* alloc);
}
