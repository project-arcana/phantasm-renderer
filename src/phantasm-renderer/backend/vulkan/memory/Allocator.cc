#include "Allocator.hh"

#include <clean-core/assert.hh>

#include "VMA.hh"

void pr::backend::vulkan::Allocator::initialize(VkPhysicalDevice physical, VkDevice device)
{
    CC_ASSERT(mAllocator == nullptr);
    VmaAllocatorCreateInfo create_info = {};
    create_info.physicalDevice = physical;
    create_info.device = device;

    vmaCreateAllocator(&create_info, &mAllocator);
}

pr::backend::vulkan::Allocator::~Allocator()
{
    if (mAllocator)
        vmaDestroyAllocator(mAllocator);
}

pr::backend::vulkan::buffer pr::backend::vulkan::Allocator::allocBuffer(uint32_t size)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    buffer res;
    vmaCreateBuffer(mAllocator, &buffer_info, &alloc_info, &res.buffer, &res.allocation, nullptr);
    return res;
}

void pr::backend::vulkan::Allocator::free(const pr::backend::vulkan::buffer& buffer)
{
    vmaDestroyBuffer(mAllocator, buffer.buffer, buffer.allocation);
}
