#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/common/native_enum.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#include <phantasm-renderer/backend/vulkan/shader.hh>

namespace pr::backend::vk
{
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
        stages_before |= util::to_pipeline_stage_dependency(change.before, change.domain_before);
        stages_after |= util::to_pipeline_stage_dependency(change.after, change.domain_after);
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

template <size_t Nimg, size_t Nbuf = 0, size_t Nmem = 0>
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
