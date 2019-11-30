#include "Device.hh"

#include <clean-core/assert.hh>
#include <clean-core/capped_vector.hh>

#include "common/verify.hh"
#include "common/zero_struct.hh"
#include "gpu_choice_util.hh"
#include "queue_util.hh"

void pr::backend::vk::Device::initialize(gpu_information const& device, VkSurfaceKHR surface, backend_config const& config)
{
    mPhysicalDevice = device.physical_device;
    CC_ASSERT(mDevice == nullptr);

    auto const active_lay_ext = get_used_device_lay_ext(device.available_layers_extensions, config, mPhysicalDevice);

    VkDeviceCreateInfo device_info;
    zero_info_struct(device_info, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
    device_info.enabledExtensionCount = uint32_t(active_lay_ext.extensions.size());
    device_info.ppEnabledExtensionNames = active_lay_ext.extensions.empty() ? nullptr : active_lay_ext.extensions.data();
    device_info.enabledLayerCount = uint32_t(active_lay_ext.layers.size());
    device_info.ppEnabledLayerNames = active_lay_ext.layers.empty() ? nullptr : active_lay_ext.layers.data();

    auto const global_queue_priority = 1.f;
    mQueueFamilies = get_chosen_queues(device.queues);
    cc::capped_vector<VkDeviceQueueCreateInfo, 3> queue_create_infos;
    for (auto i = 0u; i < 3u; ++i)
    {
        auto const queue_family_index = mQueueFamilies[i];
        if (queue_family_index == -1)
            continue;

        auto& queue_info = queue_create_infos.emplace_back();
        zero_info_struct(queue_info, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
        queue_info.queueCount = 1;
        queue_info.queueFamilyIndex = uint32_t(queue_family_index);
        queue_info.pQueuePriorities = &global_queue_priority;
    }

    device_info.pQueueCreateInfos = queue_create_infos.data();
    device_info.queueCreateInfoCount = uint32_t(queue_create_infos.size());

    // TODO
    VkPhysicalDeviceFeatures features = {};
    device_info.pEnabledFeatures = &features;

    PR_VK_VERIFY_SUCCESS(vkCreateDevice(mPhysicalDevice, &device_info, nullptr, &mDevice));

    volkLoadDevice(mDevice);

    // Query queues
    {
        if (mQueueFamilies[0] != -1)
            vkGetDeviceQueue(mDevice, uint32_t(mQueueFamilies[0]), 0, &mQueueGraphics);
        if (mQueueFamilies[1] != -1)
            vkGetDeviceQueue(mDevice, uint32_t(mQueueFamilies[1]), 0, &mQueueCompute);
        if (mQueueFamilies[2] != -1)
            vkGetDeviceQueue(mDevice, uint32_t(mQueueFamilies[2]), 0, &mQueueCopy);
    }

    // Query info
    {
        vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mInformation.memory_properties);
        vkGetPhysicalDeviceProperties(mPhysicalDevice, &mInformation.device_properties);
    }
}

void pr::backend::vk::Device::destroy()
{
    PR_VK_VERIFY_SUCCESS(vkDeviceWaitIdle(mDevice));
    vkDestroyDevice(mDevice, nullptr);
}
