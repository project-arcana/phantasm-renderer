#include "BackendVulkan.hh"

#include <clean-core/array.hh>
#include <iostream>
#include <phantasm-renderer/backend/device_tentative/window.hh>

#include "common/debug_callback.hh"
#include "common/verify.hh"
#include "common/zero_struct.hh"
#include "layer_extension_util.hh"
#include "loader/volk.hh"
#include "gpu_choice_util.hh"

void pr::backend::vk::BackendVulkan::initialize(vulkan_config const& config, device::Window& window)
{
    PR_VK_VERIFY_SUCCESS(volkInitialize());

    VkApplicationInfo app_info;
    zero_info_struct(app_info, VK_STRUCTURE_TYPE_APPLICATION_INFO);
    app_info.pApplicationName = "phantasm-renderer application";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "phantasm-renderer";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

    auto const active_lay_ext = get_used_instance_lay_ext(get_available_instance_lay_ext(), config);

    VkInstanceCreateInfo instance_info;
    zero_info_struct(instance_info, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = uint32_t(active_lay_ext.extensions.size());
    instance_info.ppEnabledExtensionNames = active_lay_ext.extensions.empty() ? nullptr : active_lay_ext.extensions.data();
    instance_info.enabledLayerCount = uint32_t(active_lay_ext.layers.size());
    instance_info.ppEnabledLayerNames = active_lay_ext.layers.empty() ? nullptr : active_lay_ext.layers.data();

    // Create the instance
    VkResult create_res = vkCreateInstance(&instance_info, nullptr, &mInstance);

    // TODO: More fine-grained error handling
    PR_VK_ASSERT_SUCCESS(create_res);

    // Load Vulkan entrypoints (instance-based)
    // TODO: volk is up to 7% slower if using this method (over i.e. volkLoadDevice(VkDevice))
    // We could possibly fastpath somehow for single-device use, or use volkLoadDeviceTable
    // See https://github.com/zeux/volk#optimizing-device-calls
    volkLoadInstance(mInstance);

    if (config.enable_validation)
    {
        // Debug callback
        createDebugMessenger();
    }

    window.createVulkanSurface(mInstance, mSurface);

    // TODO: Adapter choice
    auto const gpus = get_physical_devices(mInstance);
    for (auto const gpu : gpus)
    {
        auto const info = get_gpu_information(gpu, mSurface);

        if (info.is_suitable)
        {
            mDevice.initialize(info, mSurface, config);
            mSwapchain.initialize(mDevice, info, mSurface);
            break;
        }
    }
}

pr::backend::vk::BackendVulkan::~BackendVulkan()
{
    mSwapchain.destroy();

    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);

    mDevice.destroy();

    if (mDebugMessenger != VK_NULL_HANDLE)
        vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);

    vkDestroyInstance(mInstance, nullptr);
}

void pr::backend::vk::BackendVulkan::createDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    zero_info_struct(createInfo, VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT);
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = detail::debug_callback;
    createInfo.pUserData = nullptr;
    PR_VK_VERIFY_SUCCESS(vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger));
}
