#pragma once

#include <phantasm-renderer/enums.hh>
#include <phantasm-renderer/fwd.hh>

namespace pr::detail
{
[[nodiscard]] phi::Backend* make_backend(backend type, phi::window_handle const& window_handle, phi::backend_config const& cfg);
}
