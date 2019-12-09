#pragma once

#include <clean-core/array.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/limits.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/loader/spirv_patch_util.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
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

    struct descriptor_set_params
    {
        // one descriptor set, up to three bindings
        // 1. UAVs (STORAGE_TEXEL_BUFFER)
        // 2. SRVs (SAMPLED_IMAGE)
        // 3. Samplers
        // OR, CBVs which are padded to the sets 4,5,6,7
        // 1. CBV (UNIFORM_BUFFER_DYNAMIC)
        cc::capped_vector<VkDescriptorSetLayoutBinding, 64> bindings;

        void initialize_from_cbv();

        void add_range(VkDescriptorType type, unsigned range_start, unsigned range_size, VkShaderStageFlagBits visibility);

        void fill_in_samplers(cc::span<VkSampler const> samplers);

        [[nodiscard]] VkDescriptorSetLayout create_layout(VkDevice device) const;
    };

    cc::capped_vector<descriptor_set_params, limits::max_shader_arguments * 2> descriptor_sets;

    void initialize_from_reflection_info(cc::span<util::spirv_desc_range_info const> range_infos);
};

}

class DescriptorAllocator;

struct pipeline_layout
{
    /// The descriptor set layouts, two per shader argument:
    /// One for samplers, SRVs and UAVs, one for CBVs, shifted behind the first types
    cc::capped_vector<VkDescriptorSetLayout, limits::max_shader_arguments * 2> descriptor_set_layouts;

    /// The pipeline stages (shader domains only) which have access to
    /// the respective descriptor sets (parallel array)
    cc::capped_vector<VkPipelineStageFlags, limits::max_shader_arguments> descriptor_set_visibilities;

    /// The pipeline layout itself
    VkPipelineLayout raw_layout;

    void initialize(VkDevice device, cc::span<util::spirv_desc_range_info const> range_infos);

    void free(VkDevice device);

    [[nodiscard]] VkPipelineStageFlags get_argument_visibility(unsigned arg_i) const
    {
        if (arg_i >= limits::max_shader_arguments)
            arg_i -= limits::max_shader_arguments;

        return descriptor_set_visibilities[arg_i];
    }
};

}
