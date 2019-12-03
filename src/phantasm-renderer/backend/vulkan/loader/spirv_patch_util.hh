#pragma once

#include <cstddef>

#include <clean-core/capped_vector.hh>
#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/limits.hh>
#include <phantasm-renderer/backend/types.hh>

#include "volk.hh"

namespace pr::backend::vk::spv
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

inline constexpr auto cbv_binding_start = 0u;
inline constexpr auto srv_binding_start = 1000u;
inline constexpr auto uav_binding_start = 2000u;
inline constexpr auto sampler_binding_start = 3000u;
}

namespace pr::backend::vk::util
{
struct spirv_desc_info
{
    unsigned set;
    unsigned binding;
    VkDescriptorType type;
    VkShaderStageFlagBits visible_stage;
    VkPipelineStageFlags visible_pipeline_stage;
};
struct patched_spirv
{
    std::byte* bytecode;
    size_t bytecode_size;
};

/// we have to shift all CBVs up by [max num shader args] sets to make our API work in vulkan
/// unlike the register-to-binding shift with -fvk-[x]-shift, this cannot be done with DXC flags
/// instead we provide these helpers which use the spriv-reflect library to do the same
[[nodiscard]] arg::shader_stage create_patched_spirv(std::byte const* bytecode, size_t bytecode_size, cc::vector<spirv_desc_info>& out_desc_infos);

[[nodiscard]] inline arg::shader_stage create_patched_spirv(arg::shader_stage const& val, cc::vector<spirv_desc_info>& out_desc_infos)
{
    return create_patched_spirv(val.binary_data, val.binary_size, out_desc_infos);
}

/// purely convenience, not to be used in final backend
[[nodiscard]] arg::shader_stage create_patched_spirv_from_binary_file(char const* filename, cc::vector<spirv_desc_info>& out_desc_infos);

void free_patched_spirv(arg::shader_stage const& val);


struct spirv_desc_range_info
{
    unsigned set;
    unsigned binding_start;
    unsigned binding_size;
    VkDescriptorType type;
    VkShaderStageFlagBits visible_stages;
    // Semantically the same as visible_stages, just pre-converted (translating later would be more complicated)
    VkPipelineStageFlags visible_pipeline_stages;

    constexpr bool operator==(spirv_desc_range_info const& rhs) const noexcept
    {
        return set == rhs.set && binding_start == rhs.binding_start && binding_size == rhs.binding_size && type == rhs.type && visible_stages == rhs.visible_stages;
    }
};

/// create a sorted, deduplicated vector of descriptor range infos from an unsorted raw output from previous patches
[[nodiscard]] cc::vector<spirv_desc_range_info> merge_spirv_descriptors(cc::vector<spirv_desc_info>& desc_infos);

}
