#pragma once

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk::util
{
inline void set_viewport(VkCommandBuffer command_buf, VkExtent2D size)
{
    // This call sets a flipped viewport using the VK_KHR_Maintenance1 extension
    // which is core in Vulkan 1.1 (which we use)
    // this causes the viewport to behave as it does in OpenGL
    // see https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = float(size.height);
    viewport.width = float(size.width);
    viewport.height = -float(size.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = size;

    vkCmdSetViewport(command_buf, 0, 1, &viewport);
    vkCmdSetScissor(command_buf, 0, 1, &scissor);
}
}
