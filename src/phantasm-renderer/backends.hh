#pragma once

#include <phantasm-renderer/fwd.hh>

namespace pr
{
[[nodiscard]] phi::Backend* make_vulkan_backend(phi::window_handle const& window_handle, phi::backend_config const& cfg);
[[nodiscard]] phi::Backend* make_d3d12_backend(phi::window_handle const& window_handle, phi::backend_config const& cfg);
}
