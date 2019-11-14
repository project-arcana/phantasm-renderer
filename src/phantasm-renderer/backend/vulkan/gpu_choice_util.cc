#include "gpu_choice_util.hh"

#include <clean-core/utility.hh>
#include <cstdint>
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

VkSurfaceFormatKHR pr::backend::vk::choose_backbuffer_format(cc::span<const VkSurfaceFormatKHR> available_formats)
{
    for (auto const& f : available_formats)
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;

    return available_formats[0];
}

VkPresentModeKHR pr::backend::vk::choose_present_mode(cc::span<const VkPresentModeKHR> available_modes, bool prefer_synced)
{
    if (prefer_synced)
        for (auto const& m : available_modes)
            if (m == VK_PRESENT_MODE_MAILBOX_KHR)
                return m;

    // This mode is always available
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D pr::backend::vk::get_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, VkExtent2D fallback_size)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    else
    {
        fallback_size.width = cc::max(capabilities.minImageExtent.width, cc::min(capabilities.maxImageExtent.width, fallback_size.width));
        fallback_size.height = cc::max(capabilities.minImageExtent.height, cc::min(capabilities.maxImageExtent.height, fallback_size.height));
        return fallback_size;
    }
}