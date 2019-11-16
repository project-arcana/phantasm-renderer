#include "resource_state.hh"

VkImageMemoryBarrier pr::backend::vk::get_image_memory_barrier(VkImage image, pr::backend::resource_state before, pr::backend::resource_state after, unsigned num_mips, unsigned num_layers)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = to_image_layout(before);
    barrier.newLayout = to_image_layout(after);
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = num_mips;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = num_layers;
    barrier.srcAccessMask = to_access_flags(before);
    barrier.dstAccessMask = to_access_flags(after);
    return barrier;
}
