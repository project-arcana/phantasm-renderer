#pragma once

#include <clean-core/fwd.hh>

#include <phantasm-hardware-interface/fwd.hh>

#include <phantasm-renderer/common/api.hh>
#include <phantasm-renderer/enums.hh>

namespace pr::detail
{
PR_API phi::Backend* make_backend(backend type, cc::allocator* alloc);
}
