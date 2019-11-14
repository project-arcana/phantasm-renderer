#pragma once

#include <clean-core/array.hh>

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

    VkQueue getQueueGraphics() const { return mQueueGraphics; }
    VkQueue getQueueCompute() const { return mQueueCompute; }
    VkQueue getQueueCopy() const { return mQueueCopy; }

    int getQueueFamilyGraphics() const { return mQueueFamilies[0]; }
    int getQueueFamilyCompute() const { return mQueueFamilies[1]; }
    int getQueueFamilyCopy() const { return mQueueFamilies[2]; }

public:
    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
    VkDevice getDevice() const { return mDevice; }

private:
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice = VK_NULL_HANDLE;

    VkQueue mQueueGraphics = VK_NULL_HANDLE;
    VkQueue mQueueCompute = VK_NULL_HANDLE;
    VkQueue mQueueCopy = VK_NULL_HANDLE;
    cc::array<int, 3> mQueueFamilies;
};
}
