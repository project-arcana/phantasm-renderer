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
        [[nodiscard]] constexpr bool is_valid() const noexcept { return index != null_handle_index; }     \
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
    handle::shader_view shader_view;
    unsigned constant_buffer_offset;
};

enum class shader_domain : uint8_t
{
    // graphics
    vertex,
    hull,
    domain,
    geometry,
    pixel,

    // compute
    compute,

    // raytracing
    ray_gen,
    ray_intersect,
    ray_miss,
    ray_closest_hit,
    ray_any_hit,
};

using shader_domain_flags = uint16_t;
namespace shader_domain_flag_bits
{
enum shader_domain_flag_bits_e : shader_domain_flags
{
    unspecified = 0x0000,

    vertex = 0x0001,
    hull = 0x0002,
    domain = 0x0004,
    geometry = 0x0008,
    pixel = 0x0010,

    compute = 0x0020,

    ray_gen = 0x0040,
    ray_intersect = 0x0080,
    ray_miss = 0x0100,
    ray_closest_hit = 0x0200,
    ray_any_hit = 0x0400,

    mask_all_raytrace_stages = ray_gen | ray_intersect | ray_miss | ray_closest_hit | ray_any_hit,
    mask_all_graphics_stages = vertex | hull | domain | geometry | pixel,
};
}

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

enum class validation_level : uint8_t
{
    off,

    // D3D12: Whether to enable debug layers, requires installed D3D12 SDK
    // Vulkan: Whether to enable validation, requires installed LunarG SDK
    on,

    // D3D12: Whether to additionally enable GPU based validation (slow)
    //
    // Vulkan: Whether to additionally enable LunarG GPU-assisted validation
    //          Slow, and requires a reserved descriptor set. If your device
    //          only has 8 (max shader args * 2), like an IGP, this could fail
    //
    // Extended validation for both APIs can prevent diagnostic tools like
    // Renderdoc and NSight from working properly (PIX will work though)
    on_extended,

    // D3D12: Whether to additionally enable DRED (Device Removed Extended Data)
    //          with automatic breadcrumbs and pagefault recovery (very slow)
    //          see: https://docs.microsoft.com/en-us/windows/win32/direct3d12/use-dred
    //
    // Vulkan: No additional effect
    on_extended_dred
};

enum class present_mode : uint8_t
{
    allow_tearing,
    synced
};

using native_feature_flags = uint32_t;
namespace native_feature_flag_bits
{
/// Special features that are backend-specific, ignored if not applicable
enum native_feature_flag_bits_e : native_feature_flags
{
    none = 0,

    /// Vulkan: Enables VK_LAYER_LUNARG_api_dump (prints all API calls to stdout)
    vk_api_dump = 0x0001
};
}

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

    /// native features to enable
    native_feature_flags native_features = native_feature_flag_bits::none;

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
    unsigned max_num_cbvs = 2048;
    unsigned max_num_srvs = 2048;
    unsigned max_num_uavs = 2048;
    unsigned max_num_samplers = 1024;

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
    rg16i,
    r16i,
    rgba16u,
    rg16u,
    r16u,
    rgba16f,
    rg16f,
    r16f,
    rgba8i,
    rg8i,
    r8i,
    rgba8u,
    rg8u,
    r8u,
    rgba8un,
    rg8un,
    r8un,

    // backbuffer formats
    bgra8un,

    // depth formats
    depth32f,
    depth16un,

    // depth stencil formats
    depth32f_stencil8u,
    depth24un_stencil8u,
};

/// returns true if the format is a depth OR depth stencil format
[[nodiscard]] inline constexpr bool is_depth_format(format fmt) { return fmt >= format::depth32f; }

/// returns true if the format is a depth stencil format+
[[nodiscard]] inline constexpr bool is_depth_stencil_format(format fmt) { return fmt >= format::depth32f_stencil8u; }

/// information about a single vertex attribute
struct vertex_attribute_info
{
    char const* semantic_name;
    unsigned offset;
    format format;
};

enum class texture_dimension : uint8_t
{
    t1d,
    t2d,
    t3d
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


struct shader_view_element
{
    handle::resource resource;
    format pixel_format;
    shader_view_dimension dimension;

    struct sve_texture_info
    {
        unsigned mip_start;   ///< index of the first usable mipmap (usually: 0)
        unsigned mip_size;    ///< amount of usable mipmaps, starting from mip_start (usually: -1 / all)
        unsigned array_start; ///< index of the first usable array element [if applicable] (usually: 0)
        unsigned array_size;  ///< amount of usable array elements [if applicable]
    };

    struct sve_buffer_info
    {
        unsigned element_start;        ///< index of the first element in the buffer
        unsigned num_elements;         ///< amount of elements in the buffer
        unsigned element_stride_bytes; ///< the stride of elements in bytes
    };

    union {
        sve_texture_info texture_info;
        sve_buffer_info buffer_info;
    };

public:
    // convenience

    void init_as_null() { resource = handle::null_resource; }

    void init_as_backbuffer(handle::resource res)
    {
        resource = res;
        // cmdlist translation checks for this case and automatically chooses the right
        // format, no need to specify anything else
    }

    void init_as_tex2d(handle::resource res, format pf, bool multisampled = false, unsigned mip_start = 0, unsigned mip_size = unsigned(-1))
    {
        resource = res;
        pixel_format = pf;
        dimension = multisampled ? shader_view_dimension::texture2d_ms : shader_view_dimension::texture2d;
        texture_info.mip_start = mip_start;
        texture_info.mip_size = mip_size;
        texture_info.array_start = 0;
        texture_info.array_size = 1;
    }

    void init_as_texcube(handle::resource res, format pf)
    {
        resource = res;
        pixel_format = pf;
        dimension = shader_view_dimension::texturecube;
        texture_info.mip_start = 0;
        texture_info.mip_size = unsigned(-1);
        texture_info.array_start = 0;
        texture_info.array_size = 1;
    }

    void init_as_structured_buffer(handle::resource res, unsigned num_elements, unsigned stride_bytes)
    {
        resource = res;
        dimension = shader_view_dimension::buffer;
        buffer_info.num_elements = num_elements;
        buffer_info.element_start = 0;
        buffer_info.element_stride_bytes = stride_bytes;
    }
};

enum class sampler_filter : uint8_t
{
    min_mag_mip_point,
    min_point_mag_linear_mip_point,
    min_linear_mag_mip_point,
    min_mag_linear_mip_point,
    min_point_mag_mip_linear,
    min_linear_mag_point_mip_linear,
    min_mag_point_mip_linear,
    min_mag_mip_linear,
    anisotropic
};

enum class sampler_address_mode : uint8_t
{
    wrap,
    clamp,
    clamp_border,
    mirror
};

enum class sampler_compare_func : uint8_t
{
    never,
    less,
    equal,
    less_equal,
    greater,
    not_equal,
    greater_equal,
    always,

    disabled
};

enum class sampler_border_color : uint8_t
{
    black_transparent_float,
    black_transparent_int,
    black_float,
    black_int,
    white_float,
    white_int
};

struct sampler_config
{
    sampler_filter filter;
    sampler_address_mode address_u;
    sampler_address_mode address_v;
    sampler_address_mode address_w;
    float min_lod;
    float max_lod;
    float lod_bias;          ///< offset from the calculated MIP level (sampled = calculated + lod_bias)
    unsigned max_anisotropy; ///< maximum amount of anisotropy in [1, 16], req. sampler_filter::anisotropic
    sampler_compare_func compare_func;
    sampler_border_color border_color; ///< the border color to use, req. sampler_filter::clamp_border

    void init_default(sampler_filter filter, unsigned anisotropy = 16u)
    {
        this->filter = filter;
        address_u = sampler_address_mode::wrap;
        address_v = sampler_address_mode::wrap;
        address_w = sampler_address_mode::wrap;
        min_lod = 0.f;
        max_lod = 100000.f;
        lod_bias = 0.f;
        max_anisotropy = anisotropy;
        compare_func = sampler_compare_func::disabled;
        border_color = sampler_border_color::white_float;
    }
};

inline bool operator==(sampler_config const& lhs, sampler_config const& rhs) noexcept
{
    return lhs.filter == rhs.filter &&                 //
           lhs.address_u == rhs.address_u &&           //
           lhs.address_v == rhs.address_v &&           //
           lhs.address_w == rhs.address_w &&           //
           lhs.min_lod == rhs.min_lod &&               //
           lhs.max_lod == rhs.max_lod &&               //
           lhs.lod_bias == rhs.lod_bias &&             //
           lhs.max_anisotropy == rhs.max_anisotropy && //
           lhs.compare_func == rhs.compare_func &&     //
           lhs.border_color == rhs.border_color;       //
}


enum class rt_clear_type : uint8_t
{
    clear,
    dont_care,
    load
};

}
