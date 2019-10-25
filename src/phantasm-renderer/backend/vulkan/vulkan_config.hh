#pragma once
#ifdef PR_BACKEND_VULKAN

namespace pr::backend::vk
{
/**
 * Configuration for creating a vulkan backend
 */
struct vulkan_config
{
    bool enable_validation = false;
};
}

#endif
