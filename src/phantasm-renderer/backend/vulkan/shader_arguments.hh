#pragma once

#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/limits.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
struct uav_buffer_view
{
    VkBuffer buffer;
    uint32_t range = 0;
    uint32_t offset = 0;
};

namespace legacy
{
struct shader_argument
{
    VkBuffer cbv = nullptr;
    uint32_t cbv_view_size = 0;
    uint32_t cbv_view_offset = 0;
    cc::capped_vector<VkImageView, 8> srvs;
    cc::capped_vector<uav_buffer_view, 8> uavs;
};
}

namespace detail
{
// 1 pipeline layout - n descriptor set layouts
// 1 descriptor set layouts - n bindings
// spaces: descriptor sets
// registers: bindings
struct pipeline_layout_params
{
    // We will configure DXC to shift registers (per space) in the following way to match our bindings:
    // CBVs (b): 0          - Starts first
    // SRVs (t): 1000       - Shifted by 1k
    // UAVs (u): 2000       - Shifted by 2k
    // Samplers (s): 3000   - Shifted by 3k

    // Additionally, in order to be able to create and update VkDescriptorSets independently
    // for handle::shader_view and the single one required for the CBV, we shift CBVs up in their _set_
    // So shader arguments map as follows to sets:
    // Arg 0        SRV, UAV, Sampler: 0,   CBV: 4
    // Arg 1        SRV, UAV, Sampler: 1,   CBV: 5
    // Arg 2        SRV, UAV, Sampler: 2,   CBV: 6
    // Arg 3        SRV, UAV, Sampler: 3,   CBV: 7
    // (this is required as there are no "root descriptors" in vulkan, we do
    // it using spirv-reflect, see loader/spirv_patch_util for details)

    static constexpr auto cbv_binding_start = 0;
    static constexpr auto srv_binding_start = 1000;
    static constexpr auto uav_binding_start = 2000;
    static constexpr auto sampler_binding_start = 3000;

    struct descriptor_set_params
    {
        // one descriptor set, up to three bindings
        // 1. UAVs (STORAGE_TEXEL_BUFFER)
        // 2. SRVs (SAMPLED_IMAGE)
        // 3. Samplers
        // OR, CBVs which are padded to the sets 4,5,6,7
        // 1. CBV (UNIFORM_BUFFER_DYNAMIC)
        cc::capped_vector<VkDescriptorSetLayoutBinding, 3> bindings;

        void initialize_from_cbv();
        void initialize_from_srvs_uavs(unsigned num_srvs, unsigned num_uavs);
        void initialize_as_dummy();

        void add_implicit_sampler(VkSampler* sampler);

        [[nodiscard]] VkDescriptorSetLayout create_layout(VkDevice device) const;
    };

    cc::capped_vector<descriptor_set_params, limits::max_shader_arguments * 2> descriptor_sets;

    void initialize_from_shape(arg::shader_argument_shapes arg_shapes);
    void add_implicit_sampler_to_first_set(VkSampler* sampler);
};

}

struct pipeline_layout
{
    cc::capped_vector<VkDescriptorSetLayout, limits::max_shader_arguments * 2> descriptor_set_layouts;
    VkPipelineLayout raw_layout;

    void initialize(VkDevice device, arg::shader_argument_shapes arg_shapes);

    void free(VkDevice device);

private:
    void create_implicit_sampler(VkDevice device);

    VkSampler _implicit_sampler = nullptr;
};

class DescriptorAllocator;

struct descriptor_set_bundle
{
    cc::capped_vector<VkDescriptorSet, limits::max_shader_arguments * 2> descriptor_sets;

    void initialize(DescriptorAllocator& allocator, pipeline_layout const& layout);
    void free(DescriptorAllocator& allocator);

    /// update the n-th argument
    void update_argument(VkDevice device, uint32_t argument_index, legacy::shader_argument const& argument);

    /// bind the n-th argument at a dynamic offset
    void bind_argument(VkCommandBuffer cmd_buf, VkPipelineLayout layout, uint32_t argument_index, uint32_t cb_offset)
    {
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, argument_index, 1, &descriptor_sets[argument_index], 0, nullptr);
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, argument_index + limits::max_shader_arguments, 1,
                                &descriptor_sets[argument_index + limits::max_shader_arguments], 1, &cb_offset);
    }

    /// bind the n-th argument (no offset)
    void bind_argument(VkCommandBuffer cmd_buf, VkPipelineLayout layout, uint32_t argument_index)
    {
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, argument_index, 1, &descriptor_sets[argument_index], 0, nullptr);
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, argument_index + limits::max_shader_arguments, 1,
                                &descriptor_sets[argument_index + limits::max_shader_arguments], 0, nullptr);
    }
};

}
