#pragma once

#include <cstdint>

struct SDL_Window;
typedef struct HWND__* HWND;

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


/// generic resource (buffer, texture, render target)
PR_DEFINE_HANDLE(resource);

/// shader_view := (SRVs + UAVs + Samplers)
/// shader argument := handle::shader_view + handle::resource (CBV) + uint (CBV offset)
PR_DEFINE_HANDLE(shader_view);

/// pipeline state (vertex layout, primitive config, shaders, framebuffer formats, ...)
PR_DEFINE_HANDLE(pipeline_state);

/// recorded command list, ready to submit or discard
PR_DEFINE_HANDLE(command_list);

/// raytracing acceleration structure handle
PR_DEFINE_HANDLE(accel_struct);
}

// create an enum that is namespaced but non-class (for bitwise operations)
#define PR_DEFINE_BIT_FLAGS(_name_, _type_) \
    using _name_##_t = _type_;              \
    namespace _name_                        \
    {                                       \
    enum _name_##_e : _name_##_t

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
    ray_miss,
    ray_closest_hit,
    ray_intersect,
    ray_any_hit,
};

PR_DEFINE_BIT_FLAGS(shader_domain_flags, uint16_t){
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

/// Special features that are backend-specific, ignored if not applicable
PR_DEFINE_BIT_FLAGS(native_feature_flags, uint32_t){
    none = 0,
    /// Vulkan: Enables VK_LAYER_LUNARG_api_dump (prints all API calls to stdout)
    vk_api_dump = 0x0001,
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
    native_feature_flags_t native_features = native_feature_flags::none;

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
    unsigned max_num_accel_structs = 2048;
    unsigned max_num_raytrace_pipeline_states = 256;

    /// command list allocator size (total = #threads * #allocs/thread * #lists/alloc)
    unsigned num_cmdlist_allocators_per_thread = 5;
    unsigned num_cmdlists_per_allocator = 5;
};

/// opaque window handle
struct native_window_handle
{
    enum wh_type : uint8_t
    {
        wh_sdl,
        wh_win32_hwnd,
        wh_xlib
    };

    wh_type type;

    union {
        SDL_Window* sdl_handle;
        HWND win32_hwnd;
        struct
        {
            void* window;
            void* display;
        } xlib_handles;
    } value;

    native_window_handle(HWND hwnd) : type(wh_win32_hwnd) { value.win32_hwnd = hwnd; }
    native_window_handle(SDL_Window* sdl_window) : type(wh_sdl) { value.sdl_handle = sdl_window; }
    native_window_handle(void* xlib_win, void* xlib_display) : type(wh_xlib)
    {
        value.xlib_handles.window = xlib_win;
        value.xlib_handles.display = xlib_display;
    }
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

    resolve_src,
    resolve_dest,

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

/// returns true if the format is a depth stencil format
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

    /// receive the buffer handle from getAccelStructBuffer
    void init_as_accel_struct(handle::resource as_buffer)
    {
        resource = as_buffer;
        dimension = shader_view_dimension::raytracing_accel_struct;
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

inline constexpr bool operator==(sampler_config const& lhs, sampler_config const& rhs) noexcept
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

enum class blend_logic_op : uint8_t
{
    no_op,
    op_clear,
    op_set,
    op_copy,
    op_copy_inverted,
    op_invert,
    op_and,
    op_nand,
    op_and_inverted,
    op_and_reverse,
    op_or,
    op_nor,
    op_xor,
    op_or_reverse,
    op_or_inverted,
    op_equiv
};

enum class blend_op : uint8_t
{
    op_add,
    op_subtract,
    op_reverse_subtract,
    op_min,
    op_max
};

enum class blend_factor : uint8_t
{
    zero,
    one,
    src_color,
    inv_src_color,
    src_alpha,
    inv_src_alpha,
    dest_color,
    inv_dest_color,
    dest_alpha,
    inv_dest_alpha
};

struct render_target_config
{
    format format = format::rgba8un;
    bool blend_enable = false;
    blend_factor blend_color_src = blend_factor::one;
    blend_factor blend_color_dest = blend_factor::zero;
    blend_op blend_op_color = blend_op::op_add;
    blend_factor blend_alpha_src = blend_factor::one;
    blend_factor blend_alpha_dest = blend_factor::zero;
    blend_op blend_op_alpha = blend_op::op_add;
};

PR_DEFINE_BIT_FLAGS(accel_struct_build_flags, uint8_t){
    none = 0x00, allow_update = 0x01, allow_compaction = 0x02, prefer_fast_trace = 0x04, prefer_fast_build = 0x08, minimize_memory = 0x10,
};
}

/// geometry instance within a top level acceleration structure (layout dictated by DXR/Vulkan RT Extension)
struct accel_struct_geometry_instance
{
    /// Transform matrix, containing only the top 3 rows
    float transform[12];
    /// Instance index
    uint32_t instance_id : 24;
    /// Visibility mask
    uint32_t mask : 8;
    /// Index of the hit group which will be invoked when a ray hits the instance
    uint32_t instance_offset : 24;
    /// Instance flags, such as culling
    uint32_t flags : 8;
    /// Opaque handle of the bottom-level acceleration structure
    uint64_t native_accel_struct_handle;
};

static_assert(sizeof(accel_struct_geometry_instance) == 64, "accel_struct_geometry_instance compiles to incorrect size");

// these flags align exactly with both vulkan and d3d12, and are not translated
PR_DEFINE_BIT_FLAGS(accel_struct_instance_flags, uint32_t){none = 0x0000, triangle_cull_disable = 0x0001, triangle_front_counterclockwise = 0x0002,
                                                           force_opaque = 0x0004, force_no_opaque = 0x0008};
}

/// the size and element-strides of a shader table
struct shader_table_sizes
{
    uint32_t ray_gen_stride_bytes = 0;
    uint32_t miss_stride_bytes = 0;
    uint32_t hit_group_stride_bytes = 0;
};
}

#undef PR_DEFINE_HANDLE
#undef PR_DEFINE_BIT_FLAGS
