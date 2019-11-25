#include "shader.hh"

#include <fstream>
#include <phantasm-renderer/backend/detail/unique_buffer.hh>

#include "common/verify.hh"
#include "common/zero_struct.hh"

namespace
{
[[nodiscard]] constexpr char const* get_default_entrypoint(pr::backend::shader_domain domain)
{
    switch (domain)
    {
    case pr::backend::shader_domain::pixel:
        return "mainPS";
    case pr::backend::shader_domain::vertex:
        return "mainVS";
    case pr::backend::shader_domain::domain:
        return "mainDS";
    case pr::backend::shader_domain::hull:
        return "mainHS";
    case pr::backend::shader_domain::geometry:
        return "mainGS";
    case pr::backend::shader_domain::compute:
        return "mainCS";
    }
    return "mainPS";
}
}

pr::backend::vk::shader pr::backend::vk::create_shader_from_data(VkDevice device, void* data, size_t size, shader_domain domain, const char* entrypoint)
{
    VkShaderModuleCreateInfo shader_info;
    zero_info_struct(shader_info, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
    shader_info.codeSize = size;
    shader_info.pCode = static_cast<uint32_t const*>(data);

    shader res;
    res.domain = domain;
    res.entrypoint = entrypoint;
    PR_VK_VERIFY_SUCCESS(vkCreateShaderModule(device, &shader_info, nullptr, &res.module));
    return res;
}

pr::backend::vk::shader pr::backend::vk::create_shader_from_spirv_file(VkDevice device, const char* filename, shader_domain domain, const char* entrypoint)
{
    auto const binary = pr::backend::detail::unique_buffer::create_from_binary_file(filename);
    return create_shader_from_data(device, binary.get(), binary.size(), domain, entrypoint != nullptr ? entrypoint : get_default_entrypoint(domain));
}

VkPipelineShaderStageCreateInfo pr::backend::vk::shader::get_create_info() const
{
    VkPipelineShaderStageCreateInfo res;
    zero_info_struct(res, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
    res.stage = to_shader_stage_flags(domain);
    res.module = module;
    res.pName = entrypoint;
    return res;
}
