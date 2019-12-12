#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/command_stream.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
[[nodiscard]] VkRenderPass create_render_pass(VkDevice device, arg::framebuffer_format framebuffer, pr::primitive_pipeline_config const& config);

[[nodiscard]] VkRenderPass create_render_pass(VkDevice device, const pr::backend::cmd::begin_render_pass& begin_rp, int num_samples, cc::span<const format> override_rt_formats);

[[nodiscard]] VkPipeline create_pipeline(VkDevice device,
                                         VkRenderPass render_pass,
                                         VkPipelineLayout pipeline_layout,
                                         arg::shader_stages shaders,
                                         pr::primitive_pipeline_config const& config,
                                         cc::span<VkVertexInputAttributeDescription const> vertex_attribs,
                                         uint32_t vertex_size);

[[nodiscard]] VkPipeline create_compute_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, arg::shader_stage const& compute_shader);

[[nodiscard]] inline VkPipeline create_fullscreen_pipeline(VkDevice device, VkRenderPass render_pass, VkPipelineLayout layout, arg::shader_stages shaders)
{
    return create_pipeline(device, render_pass, layout, shaders, pr::primitive_pipeline_config{}, {}, 0);
}
}
