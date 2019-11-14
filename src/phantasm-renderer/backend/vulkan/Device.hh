#pragma once
#ifdef PR_BACKEND_VULKAN

#include "loader/volk.hh"
#include "vulkan_config.hh"

namespace pr::backend::vk
{
struct gpu_information;

class Device
{ // reference type
public:
    Device(Device const&) = delete;
    Device(Device&&) noexcept = delete;
    Device& operator=(Device const&) = delete;
    Device& operator=(Device&&) noexcept = delete;

    Device() = default;

    void initialize(gpu_information const& device, VkSurfaceKHR surface, vulkan_config const& config);
    void destroy();

public:
    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
    VkDevice getDevice() const { return mDevice; }

private:
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkQueue mQueueGraphics = VK_NULL_HANDLE;
    VkQueue mQueueCompute = VK_NULL_HANDLE;
    VkQueue mQueueCopy = VK_NULL_HANDLE;
};
}

#endif
