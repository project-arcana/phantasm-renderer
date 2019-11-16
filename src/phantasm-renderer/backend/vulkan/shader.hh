#pragma once

#include <phantasm-renderer/backend/types.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
struct shader
{
    shader_domain domain;
    VkShaderModule module;

    void free(VkDevice device) { vkDestroyShaderModule(device, module, nullptr); }

    [[nodiscard]] VkPipelineShaderStageCreateInfo get_create_info() const;
};

[[nodiscard]] inline constexpr VkShaderStageFlagBits to_shader_stage_flag(pr::backend::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::shader_domain::pixel:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case pr::backend::shader_domain::vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case pr::backend::shader_domain::domain:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case pr::backend::shader_domain::hull:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case pr::backend::shader_domain::geometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case pr::backend::shader_domain::compute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    }
}

[[nodiscard]] shader create_shader_from_data(VkDevice device, void* data, size_t size, shader_domain domain);
[[nodiscard]] shader create_shader_from_spirv_file(VkDevice device, char const* filename, shader_domain domain);
}
