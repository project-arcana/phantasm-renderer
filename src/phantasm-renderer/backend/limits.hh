#pragma once

namespace pr::backend::limits
{
/// the maximum amount of render targets per render pass, excluding the depthstencil target
/// NOTE: D3D12 only supports up to 8 render targets
inline constexpr auto max_render_targets = 8u;

/// the maximum amount of resource transitions per transition command
inline constexpr auto max_resource_transitions = 4u;

/// the maximum amount of shader arguments per draw- or compute dispatch command
/// NOTE: The Vulkan backend requires (2 * max_shader_arguments) descriptor sets,
/// most non-desktop GPUs only support a maximum of 8.
inline constexpr auto max_shader_arguments = 4u;

/// the maximum amount of samplers per shader view
inline constexpr auto max_shader_samplers = 16u;
}
