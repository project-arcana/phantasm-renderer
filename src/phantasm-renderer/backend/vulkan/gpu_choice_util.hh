#pragma once

#include <clean-core/array.hh>

#include "layer_extension_util.hh"
#include "loader/volk.hh"
#include "queue_util.hh"

namespace pr::backend::vk
{
struct gpu_information
{
    VkPhysicalDevice physical_device;
    cc::array<VkSurfaceFormatKHR> backbuffer_formats;
    cc::array<VkPresentModeKHR> present_modes;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    lay_ext_set available_layers_extensions;
    suitable_queues queues;

    bool is_suitable = false;
};

[[nodiscard]] cc::array<VkPhysicalDevice> get_physical_devices(VkInstance instance);

[[nodiscard]] gpu_information get_gpu_information(VkPhysicalDevice device, VkSurfaceKHR surface);

[[nodiscard]] VkSurfaceFormatKHR choose_backbuffer_format(cc::span<VkSurfaceFormatKHR const> available_formats);
[[nodiscard]] VkPresentModeKHR choose_present_mode(cc::span<VkPresentModeKHR const> available_modes, bool prefer_synced = true);

[[nodiscard]] VkExtent2D get_swap_extent(VkSurfaceCapabilitiesKHR const& capabilities, VkExtent2D fallback_size);

}
