#pragma once
#ifdef PR_BACKEND_VULKAN

#include <vector>

#include <phantasm-renderer/backend/Backend.hh>

#include "loader/volk.hh"
#include "vulkan_config.hh"
#include "Device.hh"

namespace pr::backend::vk
{
class BackendVulkan final : public Backend
{
public:
    void initialize(vulkan_config const& config);

    ~BackendVulkan() override;

private:
    void createDebugMessenger();

private:
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    Device mDevice;
};
}

#endif
