#pragma once

#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
// A shader payload is an array of arguments
struct shader_payload_shape
{
    // A shader argument consists of n SRVs, m UAVs plus a CBV and an offset into it
    struct shader_argument_shape
    {
        bool has_cb = false;
        unsigned num_srvs = 0;
        unsigned num_uavs = 0;
    };

    static constexpr auto max_num_arguments = 4;
    cc::capped_vector<shader_argument_shape, max_num_arguments> shader_arguments;

    bool contains_srvs() const
    {
        for (auto const& arg : shader_arguments)
            if (arg.num_srvs > 0)
                return true;

        return false;
    }
};

struct uav_buffer_view
{
    VkBuffer buffer;
    uint32_t range = 0;
    uint32_t offset = 0;
};

struct shader_argument
{
    VkBuffer cbv = VK_NULL_HANDLE;
    uint32_t cbv_view_size = 0;
    uint32_t cbv_view_offset = 0;
    cc::capped_vector<VkImageView, 8> srvs;
    cc::capped_vector<uav_buffer_view, 8> uavs;
};

namespace detail
{
// 1 pipeline - n descriptor sets
// 1 descriptor set - n bindings
// spaces: descriptor sets
// registers: bindings
struct pipeline_layout_params
{
    // We will configure DXC to shift registers (per space) in the following way to match our bindings:
    // CBVs (b): 0      - Starts first
    // UAVs (u): 1      - Directly after the single CBV
    // SRVs (t): 21     - Leaving space for 20 UAVs
    // Samplers (s): 41 - Leaving space for 20 SRVs
    static constexpr auto max_cbvs_per_space = 1;
    static constexpr auto max_uavs_per_space = 20;
    static constexpr auto max_srvs_per_space = 20;

    struct descriptor_set_params
    {
        // one descriptor set, up to three bindings
        // 1. CBV (UNIFORM_BUFFER_DYNAMIC)
        // 2. UAVs (STORAGE_TEXEL_BUFFER)
        // 3. SRVs (SAMPLED_IMAGE)
        // 4. Samplers
        cc::capped_vector<VkDescriptorSetLayoutBinding, 4> bindings;

        void initialize_from_arg_shape(shader_payload_shape::shader_argument_shape const& arg_shape);
        void add_implicit_sampler(VkSampler* sampler);

        [[nodiscard]] VkDescriptorSetLayout create_layout(VkDevice device) const;
    };

    cc::capped_vector<descriptor_set_params, 4> descriptor_sets;

    void initialize_from_shape(shader_payload_shape const& shape);
    void add_implicit_sampler_to_first_set(VkSampler* sampler);
};

}

struct pipeline_layout
{
    cc::capped_vector<VkDescriptorSetLayout, 4> descriptor_set_layouts;
    VkPipelineLayout pipeline_layout;

    void initialize(VkDevice device, shader_payload_shape const& shape);

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
    void update_argument(VkDevice device, uint32_t argument_index, shader_argument const& argument);

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
