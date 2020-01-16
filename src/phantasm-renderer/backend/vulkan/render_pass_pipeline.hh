#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/command_stream.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
[[nodiscard]] VkRenderPass create_render_pass(VkDevice device, const arg::framebuffer_config& framebuffer, pr::primitive_pipeline_config const& config);

[[nodiscard]] VkRenderPass create_render_pass(VkDevice device, const pr::backend::cmd::begin_render_pass& begin_rp, unsigned num_samples, cc::span<const format> override_rt_formats);

[[nodiscard]] VkPipeline create_pipeline(VkDevice device,
                                         VkRenderPass render_pass,
                                         VkPipelineLayout pipeline_layout,
                                         arg::graphics_shader_stages shaders,
                                         pr::primitive_pipeline_config const& config,
                                         cc::span<VkVertexInputAttributeDescription const> vertex_attribs,
                                         uint32_t vertex_size,
                                         const arg::framebuffer_config& framebuf_config);

[[nodiscard]] VkPipeline create_compute_pipeline(VkDevice device, VkPipelineLayout pipeline_layout, arg::shader_stage const& compute_shader);
}
