#pragma once

#include <cstddef>

#include <phantasm-renderer/backend/types.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
struct shader
{
    shader_domain domain;
    VkShaderModule module;
    char const* entrypoint;
    void free(VkDevice device) { vkDestroyShaderModule(device, module, nullptr); }
};

void initialize_shader(shader& s, VkDevice device, std::byte const* data, size_t size, shader_domain domain);

[[nodiscard]] VkPipelineShaderStageCreateInfo get_shader_create_info(shader const& shader);

[[nodiscard]] shader create_shader_from_data(VkDevice device, void* data, size_t size, shader_domain domain, char const* entrypoint);
[[nodiscard]] shader create_shader_from_spirv_file(VkDevice device, char const* filename, shader_domain domain, char const* entrypoint = nullptr);
}
