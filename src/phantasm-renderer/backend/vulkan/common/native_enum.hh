#pragma once

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk::util
{
[[nodiscard]] inline constexpr VkAccessFlags to_access_flags(resource_state state)
{
    using rs = resource_state;
    switch (state)
    {
    case rs::undefined:
        return 0;
    case rs::vertex_buffer:
        return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    case rs::index_buffer:
        return VK_ACCESS_INDEX_READ_BIT;

    case rs::constant_buffer:
        return VK_ACCESS_UNIFORM_READ_BIT;
    case rs::shader_resource:
        return VK_ACCESS_SHADER_READ_BIT;
    case rs::unordered_access:
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

    case rs::render_target:
        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case rs::depth_read:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    case rs::depth_write:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    case rs::indirect_argument:
        return VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

    case rs::copy_src:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case rs::copy_dest:
        return VK_ACCESS_TRANSFER_WRITE_BIT;

    case rs::present:
        return VK_ACCESS_MEMORY_READ_BIT;

    case rs::raytrace_accel_struct:
        return VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV;

    // This does not apply to access flags
    case rs::unknown:
        return 0;
    }
}

[[nodiscard]] inline constexpr VkImageLayout to_image_layout(resource_state state)
{
    using rs = resource_state;
    switch (state)
    {
    case rs::undefined:
        return VK_IMAGE_LAYOUT_UNDEFINED;

    case rs::shader_resource:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case rs::unordered_access:
        return VK_IMAGE_LAYOUT_GENERAL;

    case rs::render_target:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case rs::depth_read:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case rs::depth_write:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    case rs::copy_src:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case rs::copy_dest:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    case rs::present:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // These do not apply to image layouts
    case rs::unknown:
    case rs::vertex_buffer:
    case rs::index_buffer:
    case rs::constant_buffer:
    case rs::indirect_argument:
    case rs::raytrace_accel_struct:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

[[nodiscard]] inline constexpr VkPipelineStageFlags to_pipeline_stage_flags(pr::backend::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::shader_domain::pixel:
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case pr::backend::shader_domain::vertex:
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    case pr::backend::shader_domain::hull:
        return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
    case pr::backend::shader_domain::domain:
        return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
    case pr::backend::shader_domain::geometry:
        return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
    case pr::backend::shader_domain::compute:
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
}

[[nodiscard]] inline constexpr VkPipelineStageFlags to_pipeline_stage_dependency(resource_state state, VkPipelineStageFlags shader_flags)
{
    using rs = resource_state;
    switch (state)
    {
    case rs::vertex_buffer:
    case rs::index_buffer:
        return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

    case rs::constant_buffer:
    case rs::shader_resource:
    case rs::unordered_access:
        return shader_flags;

    case rs::render_target:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    case rs::depth_read:
    case rs::depth_write:
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    case rs::indirect_argument:
        return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

    case rs::copy_src:
    case rs::copy_dest:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;

    case rs::present:
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // TODO: Not entirely sure about this, possible BOTTOM_OF_PIPELINE instead

    case rs::raytrace_accel_struct:
        return VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV;

    // This does not apply to dependencies, conservatively use ALL_GRAPHICS
    case rs::undefined:
    case rs::unknown:
        return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    }
}

[[nodiscard]] inline constexpr VkPipelineStageFlags to_pipeline_stage_dependency(resource_state state, shader_domain domain = shader_domain::pixel)
{
    return to_pipeline_stage_dependency(state, to_pipeline_stage_flags(domain));
}

[[nodiscard]] inline constexpr VkPrimitiveTopology to_native(pr::primitive_topology topology)
{
    switch (topology)
    {
    case pr::primitive_topology::triangles:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case pr::primitive_topology::lines:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case pr::primitive_topology::points:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case pr::primitive_topology::patches:
        return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    }
}

[[nodiscard]] inline constexpr VkCompareOp to_native(pr::depth_function depth_func)
{
    switch (depth_func)
    {
    case pr::depth_function::none:
        return VK_COMPARE_OP_LESS; // sane defaults
    case pr::depth_function::less:
        return VK_COMPARE_OP_LESS;
    case pr::depth_function::less_equal:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case pr::depth_function::greater:
        return VK_COMPARE_OP_GREATER;
    case pr::depth_function::greater_equal:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case pr::depth_function::equal:
        return VK_COMPARE_OP_EQUAL;
    case pr::depth_function::not_equal:
        return VK_COMPARE_OP_NOT_EQUAL;
    case pr::depth_function::always:
        return VK_COMPARE_OP_ALWAYS;
    case pr::depth_function::never:
        return VK_COMPARE_OP_NEVER;
    }
}

[[nodiscard]] inline constexpr VkCullModeFlags to_native(pr::cull_mode cull_mode)
{
    switch (cull_mode)
    {
    case pr::cull_mode::none:
        return VK_CULL_MODE_NONE;
    case pr::cull_mode::back:
        return VK_CULL_MODE_BACK_BIT;
    case pr::cull_mode::front:
        return VK_CULL_MODE_FRONT_BIT;
    }
}


[[nodiscard]] inline constexpr VkShaderStageFlagBits to_shader_stage_flags(pr::backend::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::shader_domain::pixel:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case pr::backend::shader_domain::vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case pr::backend::shader_domain::hull:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case pr::backend::shader_domain::domain:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case pr::backend::shader_domain::geometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case pr::backend::shader_domain::compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    }
}


[[nodiscard]] inline constexpr VkDescriptorType to_native_srv_desc_type(shader_view_dimension sv_dim)
{
    switch (sv_dim)
    {
    case pr::backend::shader_view_dimension::buffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case pr::backend::shader_view_dimension::raytracing_accel_struct:
        return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
    case pr::backend::shader_view_dimension::texture1d:
    case pr::backend::shader_view_dimension::texture1d_array:
    case pr::backend::shader_view_dimension::texture2d:
    case pr::backend::shader_view_dimension::texture2d_ms:
    case pr::backend::shader_view_dimension::texture2d_array:
    case pr::backend::shader_view_dimension::texture2d_ms_array:
    case pr::backend::shader_view_dimension::texture3d:
    case pr::backend::shader_view_dimension::texturecube:
    case pr::backend::shader_view_dimension::texturecube_array:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }
}
[[nodiscard]] inline constexpr VkDescriptorType to_native_uav_desc_type(shader_view_dimension sv_dim)
{
    switch (sv_dim)
    {
    case pr::backend::shader_view_dimension::buffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case pr::backend::shader_view_dimension::texture1d:
    case pr::backend::shader_view_dimension::texture1d_array:
    case pr::backend::shader_view_dimension::texture2d:
    case pr::backend::shader_view_dimension::texture2d_ms:
    case pr::backend::shader_view_dimension::texture2d_array:
    case pr::backend::shader_view_dimension::texture2d_ms_array:
    case pr::backend::shader_view_dimension::texture3d:
    case pr::backend::shader_view_dimension::texturecube:
    case pr::backend::shader_view_dimension::texturecube_array:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

    case pr::backend::shader_view_dimension::raytracing_accel_struct:
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

[[nodiscard]] inline constexpr bool is_valid_as_uav_desc_type(shader_view_dimension sv_dim)
{
    return to_native_uav_desc_type(sv_dim) != VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

[[nodiscard]] inline constexpr VkImageViewType to_native_image_view_type(shader_view_dimension sv_dim)
{
    switch (sv_dim)
    {
    case pr::backend::shader_view_dimension::texture1d:
        return VK_IMAGE_VIEW_TYPE_1D;
    case pr::backend::shader_view_dimension::texture1d_array:
        return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    case pr::backend::shader_view_dimension::texture2d:
    case pr::backend::shader_view_dimension::texture2d_ms:
        return VK_IMAGE_VIEW_TYPE_2D;
    case pr::backend::shader_view_dimension::texture2d_array:
    case pr::backend::shader_view_dimension::texture2d_ms_array:
        return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case pr::backend::shader_view_dimension::texture3d:
        return VK_IMAGE_VIEW_TYPE_3D;
    case pr::backend::shader_view_dimension::texturecube:
        return VK_IMAGE_VIEW_TYPE_CUBE;
    case pr::backend::shader_view_dimension::texturecube_array:
        return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

    case pr::backend::shader_view_dimension::buffer:
    case pr::backend::shader_view_dimension::raytracing_accel_struct:
        CC_ASSERT(false);
        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }
}

[[nodiscard]] inline constexpr VkImageAspectFlags to_native_image_aspect(format format)
{
    if (is_depth_stencil_format(format))
    {
        return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else if (is_depth_format(format))
    {
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}


[[nodiscard]] inline constexpr VkFilter to_min_filter(sampler_filter filter)
{
    switch (filter)
    {
    case sampler_filter::min_mag_mip_point:
    case sampler_filter::min_point_mag_linear_mip_point:
    case sampler_filter::min_point_mag_mip_linear:
    case sampler_filter::min_mag_point_mip_linear:
        return VK_FILTER_NEAREST;
    case sampler_filter::min_linear_mag_mip_point:
    case sampler_filter::min_mag_linear_mip_point:
    case sampler_filter::min_linear_mag_point_mip_linear:
    case sampler_filter::min_mag_mip_linear:
    case sampler_filter::anisotropic:
        return VK_FILTER_LINEAR;
    }
}

[[nodiscard]] inline constexpr VkFilter to_mag_filter(sampler_filter filter)
{
    switch (filter)
    {
    case sampler_filter::min_mag_mip_point:
    case sampler_filter::min_linear_mag_mip_point:
    case sampler_filter::min_mag_point_mip_linear:
    case sampler_filter::min_linear_mag_point_mip_linear:
        return VK_FILTER_NEAREST;
    case sampler_filter::min_point_mag_linear_mip_point:
    case sampler_filter::min_point_mag_mip_linear:
    case sampler_filter::min_mag_linear_mip_point:
    case sampler_filter::min_mag_mip_linear:
    case sampler_filter::anisotropic:
        return VK_FILTER_LINEAR;
    }
}

[[nodiscard]] inline constexpr VkSamplerMipmapMode to_mipmap_filter(sampler_filter filter)
{
    switch (filter)
    {
    case sampler_filter::min_mag_mip_point:
    case sampler_filter::min_linear_mag_mip_point:
    case sampler_filter::min_mag_linear_mip_point:
    case sampler_filter::min_point_mag_linear_mip_point:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case sampler_filter::min_mag_point_mip_linear:
    case sampler_filter::min_linear_mag_point_mip_linear:
    case sampler_filter::min_point_mag_mip_linear:
    case sampler_filter::min_mag_mip_linear:
    case sampler_filter::anisotropic:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

[[nodiscard]] inline constexpr VkSamplerAddressMode to_native(sampler_address_mode mode)
{
    switch (mode)
    {
    case sampler_address_mode::wrap:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case sampler_address_mode::clamp:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case sampler_address_mode::clamp_border:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case sampler_address_mode::mirror:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
}

[[nodiscard]] inline constexpr VkCompareOp to_native(sampler_compare_func mode)
{
    switch (mode)
    {
    case sampler_compare_func::never:
    case sampler_compare_func::disabled:
        return VK_COMPARE_OP_NEVER;
    case sampler_compare_func::less:
        return VK_COMPARE_OP_LESS;
    case sampler_compare_func::equal:
        return VK_COMPARE_OP_EQUAL;
    case sampler_compare_func::less_equal:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case sampler_compare_func::greater:
        return VK_COMPARE_OP_GREATER;
    case sampler_compare_func::not_equal:
        return VK_COMPARE_OP_NOT_EQUAL;
    case sampler_compare_func::greater_equal:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case sampler_compare_func::always:
        return VK_COMPARE_OP_ALWAYS;
    }
}

[[nodiscard]] inline constexpr VkBorderColor to_native(sampler_border_color color)
{
    switch (color)
    {
    case sampler_border_color::black_transparent_float:
        return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    case sampler_border_color::black_transparent_int:
        return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    case sampler_border_color::black_float:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    case sampler_border_color::black_int:
        return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    case sampler_border_color::white_float:
        return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    case sampler_border_color::white_int:
        return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    }
}

[[nodiscard]] inline constexpr VkSampleCountFlagBits to_native_sample_flags(unsigned num_samples)
{
    switch (num_samples)
    {
    case 1:
        return VK_SAMPLE_COUNT_1_BIT;
    case 2:
        return VK_SAMPLE_COUNT_2_BIT;
    case 4:
        return VK_SAMPLE_COUNT_4_BIT;
    case 8:
        return VK_SAMPLE_COUNT_8_BIT;
    case 16:
        return VK_SAMPLE_COUNT_16_BIT;
    case 32:
        return VK_SAMPLE_COUNT_32_BIT;
    case 64:
        return VK_SAMPLE_COUNT_64_BIT;
    default:
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

[[nodiscard]] inline constexpr VkAttachmentLoadOp to_native(rt_clear_type clear_type)
{
    switch (clear_type)
    {
    case rt_clear_type::load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case rt_clear_type::clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case rt_clear_type::dont_care:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

[[nodiscard]] inline constexpr VkImageType to_native(texture_dimension dim)
{
    switch (dim)
    {
    case texture_dimension::t1d:
        return VK_IMAGE_TYPE_1D;
    case texture_dimension::t2d:
        return VK_IMAGE_TYPE_2D;
    case texture_dimension::t3d:
        return VK_IMAGE_TYPE_3D;
    }
}

}
