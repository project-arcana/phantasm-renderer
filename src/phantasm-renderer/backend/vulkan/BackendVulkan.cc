#include "BackendVulkan.hh"

#include <iostream>

#include <clean-core/array.hh>

#include "common/debug_callback.hh"
#include "common/verify.hh"
#include "common/zero_struct.hh"
#include "loader/volk.hh"

#include "layer_extension_util.hh"

void pr::backend::vk::BackendVulkan::initialize(vulkan_config const& config)
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

    // TODO: Debug callback

    // TODO: Adapter choice
    uint32_t num_physical_devices;
    PR_VK_VERIFY_NONERROR(vkEnumeratePhysicalDevices(mInstance, &num_physical_devices, nullptr));
    cc::array<VkPhysicalDevice> physical_devices(num_physical_devices);
    PR_VK_VERIFY_NONERROR(vkEnumeratePhysicalDevices(mInstance, &num_physical_devices, physical_devices.data()));

    //    for (auto const physical : physical_devices)
    //    {
    //        VkPhysicalDeviceProperties properties;
    //        vkGetPhysicalDeviceProperties(physical, &properties);
    //        std::cout << "Device " << properties.deviceID << ", Name: " << properties.deviceName << std::endl;
    //        std::cout << "  " << properties.driverVersion << ", " << properties.deviceType << std::endl;
    //        std::cout << " " << properties.vendorID << ", " << properties.apiVersion  << std::endl;
    //    }

    if (config.enable_validation)
    {
        createDebugMessenger();
    }

    mDevice.initialize(physical_devices[0], config);
}

pr::backend::vk::BackendVulkan::~BackendVulkan()
{
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
