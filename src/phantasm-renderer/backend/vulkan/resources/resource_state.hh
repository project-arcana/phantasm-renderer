#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/detail/resource_state.hh>

#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#include <phantasm-renderer/backend/vulkan/shader.hh>

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
        return VK_ACCESS_TRANSFER_READ_BIT;
    case rs::copy_dest:
        return VK_ACCESS_TRANSFER_WRITE_BIT;

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

[[nodiscard]] inline constexpr VkPipelineStageFlags to_pipeline_stage_flags(pr::backend::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::shader_domain::pixel:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case pr::backend::shader_domain::vertex:
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    case pr::backend::shader_domain::domain:
        return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
    case pr::backend::shader_domain::hull:
        return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    case pr::backend::shader_domain::geometry:
        return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    case pr::backend::shader_domain::compute:
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
}

[[nodiscard]] inline constexpr VkPipelineStageFlags to_pipeline_stage_dependency(resource_state state, shader_domain domain = shader_domain::pixel)
{
    using rs = resource_state;
    switch (state)
    {
    case rs::vertex_buffer:
    case rs::index_buffer:
        return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

    case rs::constant_buffer:
    case rs::shader_resource:
    case rs::unordered_access:
        return to_pipeline_stage_flags(domain);

    case rs::render_target:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    case rs::depth_read:
    case rs::depth_write:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    case rs::indirect_argument:
        return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

    case rs::copy_src:
    case rs::copy_dest:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;

    case rs::present:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO: Not entirely sure about this, possible BOTTOM_OF_PIPELINE instead

    case rs::raytrace_accel_struct:
        return VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV;

    // This does not apply to dependencies, conservatively use ALL_GRAPHICS
    case rs::undefined:
    case rs::unknown:
        return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }
}

struct state_change
{
    resource_state before;
    resource_state after;
    shader_domain domain_before = shader_domain::pixel;
    shader_domain domain_after = shader_domain::pixel;

    explicit state_change(resource_state before, resource_state after) : before(before), after(after) {}

    explicit state_change(resource_state before, shader_domain domain_before, resource_state after)
      : before(before), after(after), domain_before(domain_before)
    {
    }

    explicit state_change(resource_state before, resource_state after, shader_domain domain_after)
      : before(before), after(after), domain_after(domain_after)
    {
    }

    explicit state_change(resource_state before, shader_domain domain_before, resource_state after, shader_domain domain_after)
      : before(before), after(after), domain_before(domain_before), domain_after(domain_after)
    {
    }
};

struct stage_dependencies
{
    VkPipelineStageFlags stages_before = 0;
    VkPipelineStageFlags stages_after = 0;

    stage_dependencies() = default;
    stage_dependencies(state_change const& initial_change) { add_change(initial_change); }

    void add_change(state_change const& change)
    {
        stages_before |= to_pipeline_stage_dependency(change.before, change.domain_before);
        stages_after |= to_pipeline_stage_dependency(change.after, change.domain_after);
    }
};

[[nodiscard]] VkImageMemoryBarrier get_image_memory_barrier(
    VkImage image, state_change const& state_change, bool is_depth = false, unsigned num_mips = 1, unsigned num_layers = 1);


void submit_barriers(VkCommandBuffer cmd_buf,
                     stage_dependencies const& stage_deps,
                     cc::span<VkImageMemoryBarrier const> image_barriers,
                     cc::span<VkBufferMemoryBarrier const> buffer_barriers = {},
                     cc::span<VkMemoryBarrier const> barriers = {});

inline void submit_barriers(VkCommandBuffer cmd_buf,
                            state_change const& state_change,
                            cc::span<VkImageMemoryBarrier const> image_barriers,
                            cc::span<VkBufferMemoryBarrier const> buffer_barriers = {},
                            cc::span<VkMemoryBarrier const> barriers = {})
{
    stage_dependencies deps;
    deps.add_change(state_change);
    return submit_barriers(cmd_buf, deps, image_barriers, buffer_barriers, barriers);
}

template <size_t Nimg, size_t Nbuf, size_t Nmem>
struct barrier_bundle
{
    stage_dependencies dependencies;
    cc::capped_vector<VkImageMemoryBarrier, Nimg> barriers_img;
    cc::capped_vector<VkBufferMemoryBarrier, Nbuf> barriers_buf;
    cc::capped_vector<VkMemoryBarrier, Nmem> barriers_mem;

    void add_image_barrier(VkImage image, state_change const& state_change, bool is_depth = false, unsigned num_mips = 1, unsigned num_layers = 1)
    {
        dependencies.add_change(state_change);
        barriers_img.push_back(get_image_memory_barrier(image, state_change, is_depth, num_mips, num_layers));
    }

    /// Record contained barriers to the given cmd buffer
    void record(VkCommandBuffer cmd_buf) { submit_barriers(cmd_buf, dependencies, barriers_img, barriers_buf, barriers_mem); }

    /// Record contained barriers to the given cmd buffer, close it, and submit it on the given queue
    void submit(VkCommandBuffer cmd_buf, VkQueue queue)
    {
        record(cmd_buf);
        vkEndCommandBuffer(cmd_buf);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pNext = nullptr;
        submit_info.pWaitDstStageMask = &dependencies.stages_before;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &cmd_buf;

        PR_VK_VERIFY_SUCCESS(vkQueueSubmit(queue, 1, &submit_info, nullptr));
    }
};


}
