#pragma once

#include <clean-core/span.hh>

#include <phantasm-renderer/backend/types.hh>

#include "loader/vulkan_fwd.hh"

namespace pr::backend::vk
{
[[nodiscard]] VkSurfaceKHR create_platform_surface(VkInstance instance, native_window_handle const& window_handle);

[[nodiscard]] cc::span<char const* const> get_platform_instance_extensions();
}
