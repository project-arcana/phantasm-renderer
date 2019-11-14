#pragma once

#include <phantasm-renderer/backend/types.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
struct shader
{
    shader_domain domain;
    VkShaderModule module;

    void free(VkDevice device)
    {
        vkDestroyShaderModule(device, module, nullptr);
    }

    [[nodiscard]] VkPipelineShaderStageCreateInfo get_create_info() const;
};

[[nodiscard]] shader create_shader_from_data(VkDevice device, void* data, size_t size, shader_domain domain);
[[nodiscard]] shader create_shader_from_spirv_file(VkDevice device, char const* filename, shader_domain domain);
}
