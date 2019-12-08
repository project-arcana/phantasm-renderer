#pragma once

#include <typed-geometry/types/size.hh>

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/assets/vertex_attrib_info.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk::util
{
inline void set_viewport(VkCommandBuffer command_buf, tg::isize2 size, int start_x = 0, int start_y = 0)
{
    // vulkans viewport has a flipped y axis
    // this can be remedied by setting a negative height, see
    // https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/

    // however, this call now sets a normal, non flipped viewport,
    // we take care of flip via the -fvk-invert-y flag in dxc

    VkViewport viewport = {};
    viewport.x = float(start_x);
    viewport.y = float(start_y);
    viewport.width = float(size.width);
    viewport.height = float(size.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent.width = unsigned(size.width);
    scissor.extent.height = unsigned(size.height);

    vkCmdSetViewport(command_buf, 0, 1, &viewport);
    vkCmdSetScissor(command_buf, 0, 1, &scissor);
}

[[nodiscard]] cc::capped_vector<VkVertexInputAttributeDescription, 16> get_native_vertex_format(cc::span<vertex_attribute_info const> attrib_info);

[[nodiscard]] VkVertexInputBindingDescription get_vertex_binding(uint32_t vertex_size);
}
