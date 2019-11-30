#pragma once

#include <clean-core/array.hh>

#include <phantasm-renderer/backend/types.hh>

#include "loader/volk.hh"

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

    void initialize(gpu_information const& device, VkSurfaceKHR surface, backend_config const& config);
    void destroy();

    VkQueue getQueueGraphics() const { return mQueueGraphics; }
    VkQueue getQueueCompute() const { return mQueueCompute; }
    VkQueue getQueueCopy() const { return mQueueCopy; }

    int getQueueFamilyGraphics() const { return mQueueFamilies[0]; }
    int getQueueFamilyCompute() const { return mQueueFamilies[1]; }
    int getQueueFamilyCopy() const { return mQueueFamilies[2]; }

public:
    VkPhysicalDeviceMemoryProperties const& getMemoryProperties() const { return mInformation.memory_properties; }
    VkPhysicalDeviceProperties const& getDeviceProperties() const { return mInformation.device_properties; }

public:
    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
    VkDevice getDevice() const { return mDevice; }

private:
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice = nullptr;

    VkQueue mQueueGraphics = nullptr;
    VkQueue mQueueCompute = nullptr;
    VkQueue mQueueCopy = nullptr;
    cc::array<int, 3> mQueueFamilies;

    // Miscellaneous info
    struct {
        VkPhysicalDeviceMemoryProperties memory_properties;
        VkPhysicalDeviceProperties device_properties;
    } mInformation;
};
}
