#pragma once

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace pr::backend::vk
{
class ResourceAllocator
{
public:
    void initialize(VkPhysicalDevice physical, VkDevice device);
    void destroy();

    [[nodiscard]] VmaAllocator getAllocator() const { return mAllocator; }

private:
    VmaAllocator mAllocator = nullptr;
};

}
