#pragma once

#include <clean-core/fwd.hh>

#include <phantasm-renderer/fwd.hh>

namespace pr
{
cc::poly_unique_ptr<phi::Backend> make_vulkan_backend(phi::window_handle const& window_handle, phi::backend_config const& cfg);
cc::poly_unique_ptr<phi::Backend> make_d3d12_backend(phi::window_handle const& window_handle, phi::backend_config const& cfg);
}
