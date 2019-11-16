#include "Allocator.hh"

#include <clean-core/assert.hh>

#include "VMA.hh"

void pr::backend::vk::Allocator::initialize(VkPhysicalDevice physical, VkDevice device)
{
    CC_ASSERT(mAllocator == nullptr);

    VmaAllocatorCreateInfo create_info = {};
    create_info.physicalDevice = physical;
    create_info.device = device;

    vmaCreateAllocator(&create_info, &mAllocator);
}

void pr::backend::vk::Allocator::destroy() { vmaDestroyAllocator(mAllocator); }

pr::backend::vk::buffer pr::backend::vk::Allocator::allocBuffer(uint32_t size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    buffer res;
    vmaCreateBuffer(mAllocator, &buffer_info, &alloc_info, &res.buffer, &res.allocation, nullptr);
    return res;
}

void pr::backend::vk::Allocator::free(const pr::backend::vk::buffer& buffer) { vmaDestroyBuffer(mAllocator, buffer.buffer, buffer.allocation); }
