#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/primitive_pipeline_config.hh>

#include "loader/volk.hh"
#include "shader.hh"

namespace pr::backend::vk
{
namespace wip
{
struct framebuffer_format
{
    cc::capped_vector<VkFormat, 6> render_targets;
    cc::capped_vector<VkFormat, 1> depth_target; // a poor man's optional
};
}


[[nodiscard]] VkRenderPass create_render_pass(VkDevice device, wip::framebuffer_format const& framebuffer, pr::primitive_pipeline_config const& config);

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
