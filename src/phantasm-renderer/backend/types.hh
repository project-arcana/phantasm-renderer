#pragma once

namespace pr::backend
{
namespace handle
{
using index_t = int;
#define PR_DEFINE_HANDLE(_type_) \
    struct _type_                \
    {                            \
        index_t index;           \
    }

// Resource handles
PR_DEFINE_HANDLE(constant_buffer);
PR_DEFINE_HANDLE(resource);

// Shader arguments
PR_DEFINE_HANDLE(shader_view);

// State objects
PR_DEFINE_HANDLE(pipeline_state);
PR_DEFINE_HANDLE(root_signature);

// Dispatch objects
PR_DEFINE_HANDLE(command_list);

#undef PR_DEFINE_HANDLE
}

using index_t = int;

struct device_handle
{
    index_t index;
};

enum class shader_domain
{
    pixel,
    vertex,
    domain,
    hull,
    geometry,
    compute
};

enum class adapter_preference : char
{
    highest_vram,
    first,
    integrated,
    highest_feature_level,
    explicit_index
};

enum class adapter_vendor
{
    amd,
    intel,
    nvidia,
    unknown
};

enum class validation_level : char
{
    off,

    // D3D12: Whether to enable debug layers, requires installed D3D12 SDK
    // Vulkan: Whether to enable validation, requires installed LunarG SDK
    on,

    // D3D12: Whether to additionally enable GPU based validation (slow)
    // Vulkan: No additional effect
    on_extended
};

struct backend_config
{
    validation_level validation = validation_level::off;

    adapter_preference adapter_preference = adapter_preference::highest_vram;
    unsigned explicit_adapter_index = unsigned(-1);

    /// Enable DXR / VK raytracing features if available
    bool enable_raytracing = true;

    /// Amount of backbuffers to create
    unsigned num_backbuffers = 3;
};

namespace assets
{
enum class attribute_format
{
    rgba32f,
    rgb32f,
    rg32f,
    r32f,
    rgba32i,
    rgb32i,
    rg32i,
    r32i,
    rgba32u,
    rgb32u,
    rg32u,
    r32u,
    rgba16i,
    rgb16i,
    rg16i,
    r16i,
    rgba16u,
    rgb16u,
    rg16u,
    r16u,
    rgba8i,
    rgb8i,
    rg8i,
    r8i,
    rgba8u,
    rgb8u,
    rg8u,
    r8u
};
}
}
