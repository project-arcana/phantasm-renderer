#pragma once

#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/backend/arguments.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
struct uav_buffer_view
{
    VkBuffer buffer;
    uint32_t range = 0;
    uint32_t offset = 0;
};

namespace legacy {

struct shader_argument
{
    VkBuffer cbv = VK_NULL_HANDLE;
    uint32_t cbv_view_size = 0;
    uint32_t cbv_view_offset = 0;
    cc::capped_vector<VkImageView, 8> srvs;
    cc::capped_vector<uav_buffer_view, 8> uavs;
};
}

namespace detail
{
// 1 pipeline - n descriptor sets
// 1 descriptor set - n bindings
// spaces: descriptor sets
// registers: bindings
struct pipeline_layout_params
{
    // We will configure DXC to shift registers (per space) in the following way to match our bindings:
    // CBVs (b): 0          - Starts first
    // SRVs (t): 1000       - Shifted by 1k
    // UAVs (u): 2000       - Shifted by 2k
    // Samplers (s): 3000   - Shifted by 3k

    static constexpr auto cbv_binding_start = 0;
    static constexpr auto srv_binding_start = 1000;
    static constexpr auto uav_binding_start = 2000;
    static constexpr auto sampler_binding_start = 3000;

    struct descriptor_set_params
    {
        // one descriptor set, up to three bindings
        // 1. CBV (UNIFORM_BUFFER_DYNAMIC)
        // 2. UAVs (STORAGE_TEXEL_BUFFER)
        // 3. SRVs (SAMPLED_IMAGE)
        // 4. Samplers
        cc::capped_vector<VkDescriptorSetLayoutBinding, 4> bindings;

        void initialize_from_arg_shape(arg::shader_argument_shape const& arg_shape);
        void add_implicit_sampler(VkSampler* sampler);

        [[nodiscard]] VkDescriptorSetLayout create_layout(VkDevice device) const;
    };

    cc::capped_vector<descriptor_set_params, 4> descriptor_sets;

    void initialize_from_shape(arg::shader_argument_shapes arg_shapes);
    void add_implicit_sampler_to_first_set(VkSampler* sampler);
};

}

struct pipeline_layout
{
    cc::capped_vector<VkDescriptorSetLayout, 4> descriptor_set_layouts;
    VkPipelineLayout raw_layout;

    void initialize(VkDevice device, arg::shader_argument_shapes arg_shapes);

    void free(VkDevice device);

private:
    void create_implicit_sampler(VkDevice device);

    VkSampler _implicit_sampler = VK_NULL_HANDLE;
};

class DescriptorAllocator;

struct descriptor_set_bundle
{
    cc::capped_vector<VkDescriptorSet, 4> descriptor_sets;

    void initialize(DescriptorAllocator& allocator, pipeline_layout const& layout);
    void free(DescriptorAllocator& allocator);

    /// update the n-th argument
    void update_argument(VkDevice device, uint32_t argument_index, legacy::shader_argument const& argument);

    /// bind the n-th argument at a dynamic offset
    void bind_argument(VkCommandBuffer cmd_buf, VkPipelineLayout layout, uint32_t argument_index, uint32_t cb_offset)
    {
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, argument_index, 1, &descriptor_sets[argument_index], 1, &cb_offset);
    }

    /// bind the n-th argument (no offset)
    void bind_argument(VkCommandBuffer cmd_buf, VkPipelineLayout layout, uint32_t argument_index)
    {
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, argument_index, 1, &descriptor_sets[argument_index], 0, nullptr);
    }
};

}
