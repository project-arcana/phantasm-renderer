#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/primitive_pipeline_config.hh>
#include <phantasm-renderer/backend/arguments.hh>

#include "loader/volk.hh"
#include "shader.hh"

namespace pr::backend::vk
{
[[nodiscard]] VkRenderPass create_render_pass(VkDevice device, arg::framebuffer_format framebuffer, pr::primitive_pipeline_config const& config);

[[nodiscard]] VkPipeline create_pipeline(VkDevice device,
                                         VkRenderPass render_pass,
                                         VkPipelineLayout pipeline_layout,
                                         cc::span<shader const> shaders,
                                         pr::primitive_pipeline_config const& config,
                                         cc::span<VkVertexInputAttributeDescription const> vertex_attribs,
                                         uint32_t vertex_size);

[[nodiscard]] inline VkPipeline create_fullscreen_pipeline(VkDevice device, VkRenderPass render_pass, VkPipelineLayout layout, cc::span<shader const> shaders)
{
    return create_pipeline(device, render_pass, layout, shaders, pr::primitive_pipeline_config{}, {}, 0);
}
}
