#pragma once

#include <phantasm-renderer/backend/detail/resource_state.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
[[nodiscard]] inline constexpr VkAccessFlags to_access_flags(resource_state state)
{
    using rs = resource_state;
    switch (state)
    {
    case rs::undefined:
        return 0;
    case rs::vertex_buffer:
        return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    case rs::index_buffer:
        return VK_ACCESS_INDEX_READ_BIT;

    case rs::constant_buffer:
        return VK_ACCESS_UNIFORM_READ_BIT;
    case rs::shader_resource:
        return VK_ACCESS_SHADER_READ_BIT;
    case rs::unordered_access:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

    case rs::render_target:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case rs::depth_read:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    case rs::depth_write:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    case rs::indirect_argument:
        return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

    case rs::copy_src:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case rs::copy_dest:
        return VK_ACCESS_TRANSFER_READ_BIT;

    case rs::present:
        return VK_ACCESS_MEMORY_READ_BIT;

    case rs::raytrace_accel_struct:
        return VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;

    // This does not apply to access flags
    case rs::unknown:
        return 0;
    }
}

[[nodiscard]] inline constexpr VkImageLayout to_image_layout(resource_state state)
{
    using rs = resource_state;
    switch (state)
    {
    case rs::undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;

    case rs::shader_resource:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case rs::unordered_access:
        return VK_IMAGE_LAYOUT_GENERAL;

    case rs::render_target:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case rs::depth_read:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case rs::depth_write:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    case rs::copy_src:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case rs::copy_dest:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    case rs::present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // These do not apply to image layouts
    case rs::unknown:
    case rs::vertex_buffer:
    case rs::index_buffer:
    case rs::constant_buffer:
    case rs::indirect_argument:
    case rs::raytrace_accel_struct:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

[[nodiscard]] VkImageMemoryBarrier get_image_memory_barrier(VkImage image, resource_state before, resource_state after, unsigned num_mips = 1, unsigned num_layers = 1);

}
