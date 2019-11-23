#pragma once
#ifdef PR_BACKEND_VULKAN

#include <phantasm-renderer/backend/Backend.hh>
#include <phantasm-renderer/backend/types.hh>

#include "Device.hh"
#include "Swapchain.hh"
#include "loader/volk.hh"
#include "memory/Allocator.hh"

namespace pr::backend::device
{
class Window;
};

namespace pr::backend::vk
{
class BackendVulkan// final : public Backend
{
public:
    void initialize(backend_config const& config, device::Window& window);
    ~BackendVulkan() /*override*/;

private:
    void createDebugMessenger();

public:
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    Device mDevice;
    Swapchain mSwapchain;
    Allocator mAllocator;

private:
};
}

#endif
