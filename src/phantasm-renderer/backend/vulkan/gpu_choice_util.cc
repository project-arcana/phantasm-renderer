#include "gpu_choice_util.hh"

#include <cstdint>

#include <clean-core/utility.hh>
#include <phantasm-renderer/backend/device_tentative/window.hh>

#include "common/verify.hh"
#include "queue_util.hh"

cc::array<VkPhysicalDevice> pr::backend::vk::get_physical_devices(VkInstance instance)
{
    uint32_t num_physical_devices;
    PR_VK_VERIFY_NONERROR(vkEnumeratePhysicalDevices(instance, &num_physical_devices, nullptr));
    cc::array<VkPhysicalDevice> res(num_physical_devices);
    PR_VK_VERIFY_NONERROR(vkEnumeratePhysicalDevices(instance, &num_physical_devices, res.data()));
    return res;
}

pr::backend::vk::gpu_information pr::backend::vk::get_gpu_information(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    gpu_information res;
    res.physical_device = device;
    res.is_suitable = false;

    // queue capability
    res.queues = get_suitable_queues(device, surface);
    if (res.queues.indices_graphics.empty())
        return res;

    res.available_layers_extensions = get_available_device_lay_ext(device);

    // swapchain extensions
    if (!res.available_layers_extensions.extensions.contains(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        return res;

    // present modes and swapchain formats
    {
        uint32_t num_formats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, nullptr);
        if (num_formats == 0)
            return res;

        res.backbuffer_formats = cc::array<VkSurfaceFormatKHR>::uninitialized(num_formats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, res.backbuffer_formats.data());

        uint32_t num_present_modes;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_present_modes, nullptr);
        if (num_present_modes == 0)
            return res;

        res.present_modes = cc::array<VkPresentModeKHR>::uninitialized(num_present_modes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_present_modes, res.present_modes.data());
    }

    // surface capabilities
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &res.surface_capabilities);
    }

    res.is_suitable = true;
    return res;
}

pr::backend::vk::backbuffer_information pr::backend::vk::get_backbuffer_information(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    backbuffer_information res;

    uint32_t num_formats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, nullptr);
    CC_RUNTIME_ASSERT(num_formats != 0);
    res.backbuffer_formats = cc::array<VkSurfaceFormatKHR>::uninitialized(num_formats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &num_formats, res.backbuffer_formats.data());

    uint32_t num_present_modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_present_modes, nullptr);
    CC_RUNTIME_ASSERT(num_present_modes != 0);
    res.present_modes = cc::array<VkPresentModeKHR>::uninitialized(num_present_modes);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &num_present_modes, res.present_modes.data());

    return res;
}

VkSurfaceCapabilitiesKHR pr::backend::vk::get_surface_capabilities(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VkSurfaceCapabilitiesKHR res;
    PR_VK_VERIFY_NONERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &res));
    return res;
}

VkSurfaceFormatKHR pr::backend::vk::choose_backbuffer_format(cc::span<const VkSurfaceFormatKHR> available_formats)
{
    for (auto const& f : available_formats)
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;

    return available_formats[0];
}

VkPresentModeKHR pr::backend::vk::choose_present_mode(cc::span<const VkPresentModeKHR> available_modes, sync_mode mode)
{
    VkPresentModeKHR preferred;
    switch (mode)
    {
    case sync_mode::unsynced:
        preferred = VK_PRESENT_MODE_IMMEDIATE_KHR;
        break;
    case sync_mode::synced_fifo:
        preferred = VK_PRESENT_MODE_FIFO_KHR;
        break;
    case sync_mode::synced_mailbox:
        preferred = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
    }

    for (auto const& m : available_modes)
        if (m == preferred)
            return m;

    // This mode is always available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pr::backend::vk::get_swap_extent(const VkSurfaceCapabilitiesKHR& caps, VkExtent2D extent_hint)
{
    if (caps.currentExtent.width != UINT32_MAX)
    {
        return caps.currentExtent;
    }
    else
    {
        // Return the hint, clamped to the min/max extents
        extent_hint.width = cc::clamp(extent_hint.width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent_hint.height = cc::clamp(extent_hint.height, caps.minImageExtent.height, caps.maxImageExtent.height);
        return extent_hint;
    }
}
