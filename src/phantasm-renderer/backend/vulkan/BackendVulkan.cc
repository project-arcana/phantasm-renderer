#include "BackendVulkan.hh"
#ifdef PR_BACKEND_VULKAN

#include <string>

#include "common/verify.hh"
#include "common/zero_info_struct.hh"
#include "loader/volk.hh"

void pr::backend::vk::BackendVulkan::initialize()
{
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

void pr::backend::vk::BackendVulkan::getLayersAndExtensions(std::vector<const char*>& out_extensions, std::vector<const char*>& out_layers)
{
    std::vector<std::string> foundUniqueExtensions;
    std::vector<std::string> foundUniqueLayers;

    // Enumerate global extensions
    {
        VkResult res;
        std::vector<VkExtensionProperties> global_extensions;

        do
        {
            uint32_t res_count = 0;
            res = vkEnumerateInstanceExtensionProperties(nullptr, &res_count, nullptr);
            PR_VK_ASSERT_NONERROR(res);

            if (res_count > 0)
            {
                global_extensions.resize(res_count);
                res = vkEnumerateInstanceExtensionProperties(nullptr, &res_count, global_extensions.data());
                PR_VK_ASSERT_NONERROR(res);
            }
        } while (res == VK_INCOMPLETE);

        foundUniqueExtensions.reserve(foundUniqueExtensions.size() + global_extensions.size());

        for (auto const& ext_prop : global_extensions)
            foundUniqueExtensions.emplace_back(ext_prop.extensionName);
    }

    // Enumerate global layers
    {
        VkResult res;
        std::vector<VkLayerProperties> global_layers;

        do
        {
            uint32_t res_count;
            res = vkEnumerateInstanceLayerProperties(&res_count, nullptr);
            PR_VK_ASSERT_NONERROR(res);

            if (res_count > 0)
            {
                global_layers.resize(res_count);
                // Append to global_layers
                res = vkEnumerateInstanceLayerProperties(&res_count, &global_layers[global_layers.size() - res_count]);
                PR_VK_ASSERT_NONERROR(res);
            }

        } while (res == VK_INCOMPLETE);

        for (auto const& layer_prop : global_layers)
        {
            global_layers.emplace_back();
            auto& new_layer = global_layers.back();
        }
    }
}

#endif
