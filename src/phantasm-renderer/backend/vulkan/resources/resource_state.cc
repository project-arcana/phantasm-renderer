#include "resource_state.hh"

#include <iostream>//nocheckin

VkImageMemoryBarrier pr::backend::vk::get_image_memory_barrier(VkImage image, const state_change& state_change, VkImageAspectFlags aspect, unsigned num_mips, unsigned num_layers)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = util::to_image_layout(state_change.before);
    barrier.newLayout = util::to_image_layout(state_change.after);
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = num_mips;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = num_layers;
    barrier.srcAccessMask = util::to_access_flags(state_change.before);
    barrier.dstAccessMask = util::to_access_flags(state_change.after);
    std::cout << "Transitioning image " << image << " from " << barrier.oldLayout << " to " << barrier.newLayout << std::endl;
    return barrier;
}

VkBufferMemoryBarrier pr::backend::vk::get_buffer_memory_barrier(VkBuffer buffer, const pr::backend::vk::state_change& state_change, unsigned buffer_size)
{
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = buffer;
    barrier.srcAccessMask = util::to_access_flags(state_change.before);
    barrier.dstAccessMask = util::to_access_flags(state_change.after);
    barrier.offset = 0;
    barrier.size = buffer_size;
    return barrier;
}

void pr::backend::vk::submit_barriers(VkCommandBuffer cmd_buf,
                                      const stage_dependencies& stage_deps,
                                      cc::span<VkImageMemoryBarrier const> image_barriers,
                                      cc::span<VkBufferMemoryBarrier const> buffer_barriers,
                                      cc::span<VkMemoryBarrier const> barriers)
{
    vkCmdPipelineBarrier(cmd_buf,                                                  //
                         stage_deps.stages_before,                                 //
                         stage_deps.stages_after,                                  //
                         0,                                                        //
                         uint32_t(barriers.size()), barriers.data(),               //
                         uint32_t(buffer_barriers.size()), buffer_barriers.data(), //
                         uint32_t(image_barriers.size()), image_barriers.data()    //
    );
}
