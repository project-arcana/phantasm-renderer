#pragma once

#include <cstddef>

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

inline constexpr auto cbv_binding_start = 0;
inline constexpr auto srv_binding_start = 1000;
inline constexpr auto uav_binding_start = 2000;
inline constexpr auto sampler_binding_start = 3000;
}

namespace pr::backend::vk::util
{
struct patched_spirv
{
    std::byte* bytecode;
    size_t bytecode_size;
};

/// we have to shift all CBVs up by [max num shader args] sets to make our API work in vulkan
/// unlike the register-to-binding shift with -fvk-[x]-shift, this cannot be done with DXC flags
/// instead we provide these helpers which use the spriv-reflect library to do the same
[[nodiscard]] patched_spirv create_patched_spirv(std::byte* bytecode, size_t bytecode_size);

/// purely convenience, not to be used in final backend
[[nodiscard]] patched_spirv create_patched_spirv_from_binary_file(char const* filename);

void free_patched_spirv(patched_spirv const& val);

}
