#include "resource_state.hh"

VkImageMemoryBarrier pr::backend::vk::get_image_memory_barrier(VkImage image, const state_change& state_change, bool is_depth, unsigned num_mips, unsigned num_layers)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = to_image_layout(state_change.before);
    barrier.newLayout = to_image_layout(state_change.after);
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = is_depth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = num_mips;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = num_layers;
    barrier.srcAccessMask = to_access_flags(state_change.before);
    barrier.dstAccessMask = to_access_flags(state_change.after);
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
