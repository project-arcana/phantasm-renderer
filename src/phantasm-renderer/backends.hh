#pragma once

#include <clean-core/fwd.hh>

#include <phantasm-renderer/fwd.hh>

namespace pr
{
cc::poly_unique_ptr<backend::Backend> make_vulkan_backend(backend::window_handle const& window_handle, backend::backend_config const& cfg);
cc::poly_unique_ptr<backend::Backend> make_d3d12_backend(backend::window_handle const& window_handle, backend::backend_config const& cfg);
}
