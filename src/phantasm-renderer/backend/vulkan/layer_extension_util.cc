#include "layer_extension_util.hh"
#ifdef PR_BACKEND_VULKAN

#include <iostream>

#include "common/unique_name_set.hh"
#include "common/verify.hh"

void pr::backend::vk::layer_extension_bundle::fill_with_instance_extensions(const char* layername)
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

void pr::backend::vk::layer_extension_bundle::fill_with_device_extensions(VkPhysicalDevice device, const char* layername)
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

pr::backend::vk::lay_ext_set pr::backend::vk::get_available_instance_lay_ext()
{
    lay_ext_set available_res;

    // Add global instance layer's extensions
    {
        layer_extension_bundle global_layer;
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
            layer_extension_bundle layer;
            layer.layer_properties = layer_prop;
            layer.fill_with_instance_extensions(layer_prop.layerName);
            available_res.extensions.add(layer.extension_properties);
            available_res.layers.add(layer_prop.layerName);
        }
    }

    return available_res;
}


pr::backend::vk::lay_ext_set pr::backend::vk::get_available_device_lay_ext(VkPhysicalDevice physical)
{
    lay_ext_set available_res;

    std::vector<layer_extension_bundle> layer_extensions;

    // Add global device layer
    layer_extensions.emplace_back();

    // Enumerate device layers
    {
        uint32_t count = 0;

        PR_VK_VERIFY_SUCCESS(vkEnumerateDeviceLayerProperties(physical, &count, nullptr));

        std::vector<VkLayerProperties> layer_properties;
        layer_properties.resize(count);

        PR_VK_VERIFY_SUCCESS(vkEnumerateDeviceLayerProperties(physical, &count, layer_properties.data()));

        layer_extensions.reserve(layer_extensions.size() + layer_properties.size());
        for (auto const& layer_prop : layer_properties)
            layer_extensions.emplace_back(layer_prop);
    }

    // Track information for all additional device layers beyond the first
    for (auto i = 0u; i < layer_extensions.size(); ++i)
    {
        auto& layer = layer_extensions[i];
        if (i == 0)
        {
            layer.fill_with_device_extensions(physical, nullptr);
        }
        else
        {
            available_res.layers.add(layer.layer_properties.layerName);
            layer.fill_with_device_extensions(physical, layer.layer_properties.layerName);
        }

        available_res.extensions.add(layer.extension_properties);
    }

    return available_res;
}


pr::backend::vk::lay_ext_array pr::backend::vk::get_used_instance_lay_ext(const pr::backend::vk::lay_ext_set& available, const pr::backend::vk::vulkan_config& config)
{
    lay_ext_array used_res;

    auto const add_required_layer = [&](char const* layer_name)
    {
        if (available.layers.contains(layer_name))
        {
            used_res.layers.push_back(layer_name);
            return true;
        }
        return false;
    };

    auto const add_required_ext = [&](char const* ext_name)
    {
        if (available.extensions.contains(ext_name))
        {
            used_res.extensions.push_back(ext_name);
            return true;
        }
        return false;
    };

    // Decide upon active instance layers and extensions based on configuration and availability
    if (config.enable_validation)
    {
        if (!add_required_layer("VK_LAYER_LUNARG_standard_validation"))
        {
            if (!add_required_layer("VK_LAYER_KHRONOS_validation"))
            {
                std::cerr << "[pr][vk] Validation enabled, but no layers available on Vulkan instance" << std::endl;
                std::cerr << "[pr][vk] Download the LunarG SDK for your operating system," << std::endl;
                std::cerr << "[pr][vk] then set the VK_LAYER_PATH environment variable to the" << std::endl;
                std::cerr << "[pr][vk] absolute path towards <sdk>/x86_64/etc/vulkan/explicit_layer.d/" << std::endl;
            }
        }

        if (!add_required_ext("VK_EXT_debug_utils"))
        {
            std::cerr << "Missing debug utils extension" << std::endl;
        }
    }

    // TODO: platform extensions
    // these must be sourced from device-abstraction somehow
    if (!add_required_ext("VK_KHR_surface"))
    {
        std::cerr << "Missing surface extension" << std::endl;
    }


    return used_res;
}

pr::backend::vk::lay_ext_array pr::backend::vk::get_used_device_lay_ext(const pr::backend::vk::lay_ext_set& available,
                                                                        const pr::backend::vk::vulkan_config& config,
                                                                        VkPhysicalDevice physical)
{
    lay_ext_array used_res;

    // Decide upon active device layers and extensions based on configuration and availability
    // TODO: Everything

    return used_res;
}

#endif
