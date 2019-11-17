#include "shader_arguments.hh"

#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/resources/resource_state.hh>
#include <phantasm-renderer/backend/vulkan/resources/resource_view.hh>

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::initialize_from_arg_shape(const pr::backend::vk::shader_payload_shape::shader_argument_shape& arg_shape)
{
    auto const argument_visibility = VK_SHADER_STAGE_ALL_GRAPHICS; // NOTE: Eventually arguments could be constrained to stages

    if (arg_shape.has_cb)
    {
        VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
        binding = {};
        binding.binding = 0u; // CBV always in (0)
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        binding.descriptorCount = 1;
        binding.stageFlags = argument_visibility;
        binding.pImmutableSamplers = nullptr; // Optional
    }

    if (arg_shape.num_uavs > 0)
    {
        VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
        binding = {};
        binding.binding = max_cbvs_per_space; // UAVs start behind the CBV

        // NOTE: UAVs map the following way to SPIR-V:
        // RWBuffer<T> -> VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        // RWTextureX<T> -> VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        // In other words this is an incomplete way of mapping
        // See https://github.com/microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst#textures

        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        binding.descriptorCount = arg_shape.num_uavs;
        binding.stageFlags = argument_visibility;
        binding.pImmutableSamplers = nullptr; // Optional
    }

    if (arg_shape.num_srvs > 0)
    {
        VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
        binding = {};
        binding.binding = max_cbvs_per_space + max_uavs_per_space; // SRVs start behind the CBV and the UAVs
        binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        binding.descriptorCount = arg_shape.num_srvs;
        binding.stageFlags = argument_visibility;
        binding.pImmutableSamplers = nullptr; // Optional
    }
}

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::add_implicit_sampler(VkSampler* sampler)
{
    VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
    binding = {};
    binding.binding = max_cbvs_per_space + max_uavs_per_space + max_srvs_per_space; // The implicit sampler comes at the very last position
    binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    binding.pImmutableSamplers = sampler;
}

VkDescriptorSetLayout pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::create_layout(VkDevice device) const
{
    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = uint32_t(bindings.size());
    layout_info.pBindings = bindings.data();

    VkDescriptorSetLayout res;
    PR_VK_VERIFY_SUCCESS(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &res));
    return res;
}

void pr::backend::vk::pipeline_layout::initialize(VkDevice device, const pr::backend::vk::shader_payload_shape& shape)
{
    detail::pipeline_layout_params params;
    params.initialize_from_shape(shape);

    if (shape.contains_srvs())
    {
        create_implicit_sampler(device);
        params.add_implicit_sampler_to_first_set(&_implicit_sampler);
    }

    for (auto const& param_set : params.descriptor_sets)
    {
        descriptor_set_layouts.push_back(param_set.create_layout(device));
    }

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = uint32_t(descriptor_set_layouts.size());
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    layout_info.pushConstantRangeCount = 0;
    layout_info.pPushConstantRanges = nullptr;

    PR_VK_VERIFY_SUCCESS(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout));
}

void pr::backend::vk::pipeline_layout::free(VkDevice device)
{
    for (auto const layout : descriptor_set_layouts)
        vkDestroyDescriptorSetLayout(device, layout, nullptr);

    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

    if (_implicit_sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, _implicit_sampler, nullptr);
    }
}

void pr::backend::vk::pipeline_layout::create_implicit_sampler(VkDevice device)
{
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.minLod = -1000;
    info.maxLod = 1000;
    info.maxAnisotropy = 16.0f;
    PR_VK_VERIFY_SUCCESS(vkCreateSampler(device, &info, nullptr, &_implicit_sampler));
}

void pr::backend::vk::detail::pipeline_layout_params::initialize_from_shape(const pr::backend::vk::shader_payload_shape& shape)
{
    for (auto const& arg_shape : shape.shader_arguments)
    {
        descriptor_sets.emplace_back();
        descriptor_sets.back().initialize_from_arg_shape(arg_shape);
    }
}

void pr::backend::vk::detail::pipeline_layout_params::add_implicit_sampler_to_first_set(VkSampler* sampler)
{
    descriptor_sets[0].add_implicit_sampler(sampler);
}

void pr::backend::vk::descriptor_set_bundle::initialize(pr::backend::vk::DescriptorAllocator& allocator, const pr::backend::vk::pipeline_layout& layout)
{
    for (auto const& set_layout : layout.descriptor_set_layouts)
    {
        auto& set = descriptor_sets.emplace_back();
        allocator.allocDescriptor(set_layout, set);
    }
}

void pr::backend::vk::descriptor_set_bundle::free(pr::backend::vk::DescriptorAllocator& allocator)
{
    for (auto desc_set : descriptor_sets)
        allocator.free(desc_set);
}

void pr::backend::vk::descriptor_set_bundle::update_argument(VkDevice device, uint32_t argument_index, const pr::backend::vk::shader_argument& argument)
{
    auto const& desc_set = descriptor_sets[argument_index];

    cc::capped_vector<VkWriteDescriptorSet, 3> writes;
    VkDescriptorBufferInfo cbv_info;

    if (argument.cbv != VK_NULL_HANDLE)
    {
        // bind CBV

        cbv_info = {};
        cbv_info.buffer = argument.cbv;
        cbv_info.offset = argument.cbv_view_offset;
        cbv_info.range = argument.cbv_view_size;

        auto& write = writes.emplace_back();
        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = desc_set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.descriptorCount = 1; // Just one CBV
        write.pBufferInfo = &cbv_info;
        write.dstArrayElement = 0;
        write.dstBinding = 0; // CBVs always in binding 0
    }

    cc::capped_vector<VkDescriptorBufferInfo, 8> uav_infos;

    if (!argument.uavs.empty())
    {
        for (auto uav : argument.uavs)
        {
            auto& srv_info = uav_infos.emplace_back();
            srv_info.buffer = uav.buffer;
            srv_info.offset = uav.offset;
            srv_info.range = uav.range;
        }

        auto& write = writes.emplace_back();
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = desc_set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        write.descriptorCount = uint32_t(uav_infos.size());
        write.pBufferInfo = uav_infos.data();
        write.dstArrayElement = 0;
        write.dstBinding = detail::pipeline_layout_params::max_cbvs_per_space; // UAVs start behind the CBV
    }

    cc::capped_vector<VkDescriptorImageInfo, 8> srv_infos;

    if (!argument.srvs.empty())
    {
        for (auto srv : argument.srvs)
        {
            auto& srv_info = srv_infos.emplace_back();
            srv_info.imageView = srv;
            srv_info.imageLayout = to_image_layout(resource_state::shader_resource);
        }

        auto& write = writes.emplace_back();
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = desc_set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.descriptorCount = uint32_t(srv_infos.size());
        write.pImageInfo = srv_infos.data();
        write.dstArrayElement = 0;
        write.dstBinding = detail::pipeline_layout_params::max_cbvs_per_space + detail::pipeline_layout_params::max_uavs_per_space; // SRVs start behind the CBV and the UAVs
    }

    vkUpdateDescriptorSets(device, uint32_t(writes.size()), writes.data(), 0, nullptr);
}
