#include "shader.hh"

#include <fstream>
#include <phantasm-renderer/backend/detail/unique_buffer.hh>

#include "common/verify.hh"
#include "common/zero_struct.hh"

pr::backend::vk::shader pr::backend::vk::create_shader_from_data(VkDevice device, void* data, size_t size, shader_domain domain)
{
    VkShaderModuleCreateInfo shader_info;
    zero_info_struct(shader_info, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    shader_info.codeSize = size;
    shader_info.pCode = static_cast<uint32_t const*>(data);

    shader res;
    res.domain = domain;
    PR_VK_VERIFY_SUCCESS(vkCreateShaderModule(device, &shader_info, nullptr, &res.module));
    return res;
}

pr::backend::vk::shader pr::backend::vk::create_shader_from_spirv_file(VkDevice device, const char* filename, shader_domain domain)
{
    auto const binary = pr::backend::detail::unique_buffer::create_from_binary_file(filename);
    return create_shader_from_data(device, binary.get(), binary.size(), domain);
}

VkPipelineShaderStageCreateInfo pr::backend::vk::shader::get_create_info() const
{
    VkPipelineShaderStageCreateInfo res;
    zero_info_struct(res, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
    res.stage = to_shader_stage_flag(domain);
    res.module = module;
    res.pName = "main";
    return res;
}
