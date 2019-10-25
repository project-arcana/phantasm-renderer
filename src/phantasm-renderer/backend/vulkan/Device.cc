#include "Device.hh"
#ifdef PR_BACKEND_VULKAN

#include <clean-core/assert.hh>

#include "common/zero_struct.hh"
#include "layer_extension_util.hh"

pr::backend::vk::Device::Device(VkPhysicalDevice physical) : mPhysicalDevice(physical) {}

void pr::backend::vk::Device::initialize(vulkan_config const& config)
{
    CC_ASSERT(mDevice == VK_NULL_HANDLE);

    auto const active_lay_ext = get_used_device_lay_ext(get_available_device_lay_ext(mPhysicalDevice), config, mPhysicalDevice);

    VkDeviceCreateInfo device_info;
    zero_info_struct(device_info, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
    device_info.enabledExtensionCount = uint32_t(active_lay_ext.extensions.size());
    device_info.ppEnabledExtensionNames = active_lay_ext.extensions.empty() ? nullptr : active_lay_ext.extensions.data();
    device_info.enabledLayerCount = uint32_t(active_lay_ext.layers.size());
    device_info.ppEnabledLayerNames = active_lay_ext.layers.empty() ? nullptr : active_lay_ext.layers.data();

    // TODO: Queue info query, possibly create queue_util.hh header analogously
}

#endif
