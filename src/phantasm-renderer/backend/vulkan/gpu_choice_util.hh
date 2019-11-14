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

struct backbuffer_information
{
    cc::array<VkSurfaceFormatKHR> backbuffer_formats;
    cc::array<VkPresentModeKHR> present_modes;
};

[[nodiscard]] cc::array<VkPhysicalDevice> get_physical_devices(VkInstance instance);

/// receive full information about a GPU, relatively slow
[[nodiscard]] gpu_information get_gpu_information(VkPhysicalDevice device, VkSurfaceKHR surface);

/// receive only backbuffer-related information
[[nodiscard]] backbuffer_information get_backbuffer_information(VkPhysicalDevice device, VkSurfaceKHR surface);

/// receive surface capabilities
[[nodiscard]] VkSurfaceCapabilitiesKHR get_surface_capabilities(VkPhysicalDevice device, VkSurfaceKHR surface);

[[nodiscard]] VkSurfaceFormatKHR choose_backbuffer_format(cc::span<VkSurfaceFormatKHR const> available_formats);
[[nodiscard]] VkPresentModeKHR choose_present_mode(cc::span<VkPresentModeKHR const> available_modes, bool prefer_synced = true);

[[nodiscard]] VkExtent2D get_swap_extent(VkSurfaceCapabilitiesKHR const& capabilities, VkExtent2D fallback_size);

[[nodiscard]] inline VkSurfaceTransformFlagBitsKHR choose_identity_transform(VkSurfaceCapabilitiesKHR const& capabilities)
{
    return (capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : capabilities.currentTransform;
}

[[nodiscard]] inline VkCompositeAlphaFlagBitsKHR choose_alpha_mode(VkSurfaceCapabilitiesKHR const& capabilities)
{
    for (auto flag : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR})
        if (capabilities.supportedCompositeAlpha & flag)
            return flag;

    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

}
