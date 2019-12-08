#include "ResourceAllocator.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/vulkan/common/verify.hh>

#include "VMA.hh"

void pr::backend::vk::ResourceAllocator::initialize(VkPhysicalDevice physical, VkDevice device)
{
    CC_ASSERT(mAllocator == nullptr);

    VmaAllocatorCreateInfo create_info = {};
    create_info.physicalDevice = physical;
    create_info.device = device;

    PR_VK_VERIFY_SUCCESS(vmaCreateAllocator(&create_info, &mAllocator));
}

void pr::backend::vk::ResourceAllocator::destroy() { vmaDestroyAllocator(mAllocator); }
