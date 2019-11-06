#include "BackendVulkan.hh"
#ifdef PR_BACKEND_VULKAN

#include <iostream>

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
    app_info.applicationVersion = 0;
    app_info.pEngineName = "phantasm-renderer";
    app_info.engineVersion = 0;
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
}

pr::backend::vk::BackendVulkan::~BackendVulkan() { vkDestroyInstance(mInstance, nullptr); }


#endif
