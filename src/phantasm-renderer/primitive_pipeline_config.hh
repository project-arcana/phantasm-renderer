#pragma once

namespace pr
{
// see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPrimitiveTopology.html
enum class primitive_topology
{
    // TODO
    triangles
};

// see https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkGraphicsPipelineCreateInfo.html
struct primitive_pipeline_config
{
    primitive_topology topology = primitive_topology::triangles;
};
}
