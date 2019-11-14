#pragma once

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk::util
{
inline void set_viewport(VkCommandBuffer command_buf, VkExtent2D size)
{
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = float(size.width);
    viewport.height = float(size.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = size;

    vkCmdSetViewport(command_buf, 0, 1, &viewport);
    vkCmdSetScissor(command_buf, 0, 1, &scissor);
}

}
