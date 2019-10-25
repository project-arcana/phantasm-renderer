#pragma once
#ifdef PR_BACKEND_VULKAN

#include <string>
#include <vector>

#include <clean-core/span.hh>

#include "common/unique_name_set.hh"
#include "common/zero_struct.hh"
#include "loader/volk.hh"

namespace pr::backend::vk
{
struct layer_extension
{
    VkLayerProperties layer_properties;
    std::vector<VkExtensionProperties> extension_properties;

    layer_extension() { zero_struct(layer_properties); }

    void fill_with_instance_extensions(char const* layername = nullptr);
    void fill_with_device_extensions(VkPhysicalDevice device, char const* layername = nullptr);
};

// TODO: Name
struct available_layers_and_extensions
{
    unique_name_set<vk_name_type::layer> layers;
    unique_name_set<vk_name_type::extension> extensions;
};

[[nodiscard]] available_layers_and_extensions get_available_instance_layers_and_extensions();
}

#endif
