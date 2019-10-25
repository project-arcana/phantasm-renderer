#include "layer_extension_util.hh"
#ifdef PR_BACKEND_VULKAN

#include <iostream>

#include "common/unique_name_set.hh"
#include "common/verify.hh"

void pr::backend::vk::layer_extension::fill_with_instance_extensions(const char* layername)
{
    VkResult res;
    do
    {
        uint32_t res_count = 0;
        res = vkEnumerateInstanceExtensionProperties(layername, &res_count, nullptr);
        PR_VK_ASSERT_NONERROR(res);

        if (res_count > 0)
        {
            extension_properties.resize(res_count);
            res = vkEnumerateInstanceExtensionProperties(layername, &res_count, extension_properties.data());
            PR_VK_ASSERT_NONERROR(res);
        }
    } while (res == VK_INCOMPLETE);
}

void pr::backend::vk::layer_extension::fill_with_device_extensions(VkPhysicalDevice device, const char* layername)
{
    VkResult res;
    do
    {
        uint32_t res_count = 0;
        res = vkEnumerateDeviceExtensionProperties(device, layername, &res_count, nullptr);
        PR_VK_ASSERT_NONERROR(res);

        if (res_count > 0)
        {
            extension_properties.resize(res_count);
            res = vkEnumerateDeviceExtensionProperties(device, layername, &res_count, extension_properties.data());
            PR_VK_ASSERT_NONERROR(res);
        }
    } while (res == VK_INCOMPLETE);
}

pr::backend::vk::available_layers_and_extensions pr::backend::vk::get_available_instance_layers_and_extensions()
{
    available_layers_and_extensions available_res;

    // Add global layer's extensions
    {
        layer_extension global_layer;
        global_layer.fill_with_instance_extensions(nullptr);

        available_res.extensions.add(global_layer.extension_properties);
    }

    // Enumerate instance layers
    {
        VkResult res;
        std::vector<VkLayerProperties> global_layer_properties;

        do
        {
            uint32_t res_count;
            res = vkEnumerateInstanceLayerProperties(&res_count, nullptr);
            PR_VK_ASSERT_NONERROR(res);

            if (res_count > 0)
            {
                global_layer_properties.resize(global_layer_properties.size() + res_count);
                // Append to global_layers
                res = vkEnumerateInstanceLayerProperties(&res_count, &global_layer_properties[global_layer_properties.size() - res_count]);
                PR_VK_ASSERT_NONERROR(res);
            }

        } while (res == VK_INCOMPLETE);

        for (auto const& layer_prop : global_layer_properties)
        {
            layer_extension layer;
            layer.layer_properties = layer_prop;
            layer.fill_with_instance_extensions(layer_prop.layerName);

            available_res.extensions.add(layer.extension_properties);
            available_res.layers.add(layer_prop.layerName);
        }
    }

    return available_res;
}


#endif
