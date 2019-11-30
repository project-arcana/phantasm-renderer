#pragma once

#include <cstdint>

namespace pr::backend
{
namespace handle
{
using index_t = int32_t;
inline constexpr index_t null_handle_index = index_t(-1);
#define PR_DEFINE_HANDLE(_type_)                                                                          \
    struct _type_                                                                                         \
    {                                                                                                     \
        index_t index;                                                                                    \
        [[nodiscard]] constexpr bool is_valid() const { return index != null_handle_index; }              \
        [[nodiscard]] constexpr bool operator==(_type_ rhs) const noexcept { return index == rhs.index; } \
        [[nodiscard]] constexpr bool operator!=(_type_ rhs) const noexcept { return index != rhs.index; } \
    };                                                                                                    \
    inline constexpr _type_ null_##_type_ = {null_handle_index}


/// generic resource (Image, Buffer, RT)
PR_DEFINE_HANDLE(resource);

/// shader arguments := handle::shader_view (SRVs + UAVs) + handle::resource (CBV) + uint (CBV offset)
PR_DEFINE_HANDLE(shader_view);

/// pipeline state (vertex layout, primitive config, shaders, framebuffer formats)
PR_DEFINE_HANDLE(pipeline_state);

/// command list handle, returned from compiles
PR_DEFINE_HANDLE(command_list);

#undef PR_DEFINE_HANDLE
}

struct shader_argument
{
    handle::resource constant_buffer;
    unsigned constant_buffer_offset;
    handle::shader_view shader_view;
};

enum class shader_domain : uint8_t
{
    pixel,
    vertex,
    domain,
    hull,
    geometry,
    compute
};

enum class queue_type : uint8_t
{
    graphics,
    copy,
    compute
};

enum class adapter_preference : uint8_t
{
    highest_vram,
    first,
    integrated,
    highest_feature_level,
    explicit_index
};

enum class adapter_vendor : uint8_t
{
    amd,
    intel,
    nvidia,
    unknown
};

enum class validation_level : uint8_t
{
    off,

    // D3D12: Whether to enable debug layers, requires installed D3D12 SDK
    // Vulkan: Whether to enable validation, requires installed LunarG SDK
    on,

    // D3D12: Whether to additionally enable GPU based validation (slow)
    // Vulkan: No additional effect
    on_extended
};

enum class present_mode : uint8_t
{
    allow_tearing,
    synced
};

struct backend_config
{
    /// whether to enable API-level validations
    validation_level validation = validation_level::off;

    /// the swapchain presentation mode (note: synced does not mean refreshrate-limited)
    /// Also note: under some circumstances, synced might force a refreshrate limit (Nvidia optimus + windowed on d3d12, etc.)
    present_mode present_mode = present_mode::synced;

    /// the strategy for choosing a physical GPU
    adapter_preference adapter_preference = adapter_preference::highest_vram;
    unsigned explicit_adapter_index = unsigned(-1);

    /// whether to enable DXR / VK raytracing features if available
    bool enable_raytracing = true;

    /// amount of backbuffers to create
    unsigned num_backbuffers = 3;

    /// amount of threads to accomodate
    /// backend calls must only be made from <= [num_threads] unique OS threads
    unsigned num_threads = 1;

    /// resource limits
    unsigned max_num_resources = 2048;
    unsigned max_num_pipeline_states = 1024;
    unsigned max_num_shader_view_elements = 4096;

    /// command list allocator size (total = #threads * #allocs/thread * #lists/alloc)
    unsigned num_cmdlist_allocators_per_thread = 5;
    unsigned num_cmdlists_per_allocator = 5;
};

// Maps to
// D3D12: resource states
// Vulkan: access masks, image layouts and pipeline stage dependencies
enum class resource_state : uint8_t
{
    // unknown to pr
    unknown,
    // undefined in API semantics
    undefined,

    vertex_buffer,
    index_buffer,

    constant_buffer,
    shader_resource,
    unordered_access,

    render_target,
    depth_read,
    depth_write,

    indirect_argument,

    copy_src,
    copy_dest,

    present,

    raytrace_accel_struct,
};

// Maps to DXGI_FORMAT and VkFormat
// [f]loat, [i]nt, [u]int, [un]orm
enum class format : uint8_t
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
    rgba16f,
    rgb16f,
    rg16f,
    r16f,
    rgba8i,
    rgb8i,
    rg8i,
    r8i,
    rgba8u,
    rgb8u,
    rg8u,
    r8u,

    // backbuffer formats
    rgba8un,

    // depth stencil formats
    depth32f,
    depth16un,
    depth32f_stencil8u,
    depth24un_stencil8u,
};

[[nodiscard]] inline constexpr bool is_depth_format(format fmt) { return fmt >= format::depth32f; }

/// information about a single vertex attribute
struct vertex_attribute_info
{
    char const* semantic_name;
    unsigned offset;
    format format;
};


enum class shader_view_dimension : uint8_t
{
    buffer,
    texture1d,
    texture1d_array,
    texture2d,
    texture2d_ms,
    texture2d_array,
    texture2d_ms_array,
    texture3d,
    texturecube,
    texturecube_array,
    raytracing_accel_struct
};
}
