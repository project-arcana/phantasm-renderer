#pragma once
#ifdef PR_BACKEND_VULKAN

#include <phantasm-renderer/backend/Backend.hh>
#include <vector>

#include "Device.hh"
#include "loader/volk.hh"
#include "Swapchain.hh"
#include "vulkan_config.hh"

namespace pr::backend::device
{
class Window;
};

namespace pr::backend::vk
{
class BackendVulkan final : public Backend
{
public:
    void initialize(vulkan_config const& config, device::Window& window);

    ~BackendVulkan() override;

private:
    void createDebugMessenger();

private:
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    Device mDevice;
    Swapchain mSwapchain;
};
}

#endif
