#pragma once

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace pr::backend::vk
{
struct buffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};


class Allocator
{
public:
    void initialize(VkPhysicalDevice physical, VkDevice device);
    void destroy();

    [[nodiscard]] buffer allocBuffer(uint32_t size, VkBufferUsageFlags usage);

    void free(buffer const& buffer);

private:
    VmaAllocator mAllocator = nullptr;
};

}
