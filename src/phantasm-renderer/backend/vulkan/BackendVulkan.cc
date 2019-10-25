#include "BackendVulkan.hh"
#ifdef PR_BACKEND_VULKAN

#include <iostream>

#include "common/verify.hh"
#include "common/zero_struct.hh"
#include "loader/volk.hh"

#include "layer_extension_util.hh"

void pr::backend::vk::BackendVulkan::initialize()
{
    PR_VK_VERIFY_SUCCESS(volkInitialize());

    VkApplicationInfo app_info;
    zero_info_struct(app_info, VK_STRUCTURE_TYPE_APPLICATION_INFO);
    app_info.pApplicationName = "phantasm-renderer application";
    app_info.applicationVersion = 0;
    app_info.pEngineName = "phantasm-renderer";
    app_info.engineVersion = 0;
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instance_info;
    zero_info_struct(instance_info, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
    instance_info.pApplicationInfo = &app_info;
}


#endif
