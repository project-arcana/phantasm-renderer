#include "shader_arguments.hh"

#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/resources/resource_state.hh>
#include <phantasm-renderer/backend/vulkan/resources/resource_view.hh>

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::initialize_from_cbv()
{
    constexpr auto argument_visibility = VK_SHADER_STAGE_ALL_GRAPHICS; // NOTE: Eventually arguments could be constrained to stages

    VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
    binding = {};
    binding.binding = spv::cbv_binding_start; // CBV always in (0)
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    binding.descriptorCount = 1;
    binding.stageFlags = argument_visibility;
    binding.pImmutableSamplers = nullptr; // Optional
}

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::initialize_from_srvs_uavs(unsigned num_srvs, unsigned num_uavs)
{
    constexpr auto argument_visibility = VK_SHADER_STAGE_ALL_GRAPHICS; // NOTE: Eventually arguments could be constrained to stages

    if (num_uavs > 0)
    {
        VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
        binding = {};
        binding.binding = spv::uav_binding_start;

        // NOTE: UAVs map the following way to SPIR-V:
        // RWBuffer<T> -> VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        // RWTextureX<T> -> VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        // In other words this is an incomplete way of mapping
        // See https://github.com/microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst#textures

        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
        binding.descriptorCount = num_uavs;
        binding.stageFlags = argument_visibility;
        binding.pImmutableSamplers = nullptr; // Optional
    }

    if (num_srvs > 0)
    {
        VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
        binding = {};
        binding.binding = spv::srv_binding_start;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        binding.descriptorCount = num_srvs;
        binding.stageFlags = argument_visibility;
        binding.pImmutableSamplers = nullptr; // Optional
    }
}

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::add_range(VkDescriptorType type, unsigned range_start, unsigned range_size, VkShaderStageFlagBits visibility)
{
    VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
    binding = {};
    binding.binding = range_start;
    binding.descriptorType = type;
    binding.descriptorCount = range_size;
    binding.stageFlags = visibility;
    binding.pImmutableSamplers = nullptr; // Optional
}

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::add_implicit_sampler(VkSampler* sampler)
{
    VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
    binding = {};
    binding.binding = spv::sampler_binding_start;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    binding.pImmutableSamplers = sampler;
}

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::fill_in_implicit_sampler(VkSampler* sampler)
{
    for (auto& binding : bindings)
    {
        if (binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
        {
            binding.pImmutableSamplers = sampler;
            return;
        }
    }

    CC_ASSERT(false && "Failed to fill in implicit sampler");
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

void pr::backend::vk::pipeline_layout::initialize(VkDevice device, pr::backend::arg::shader_argument_shapes arg_shapes)
{
    detail::pipeline_layout_params params;
    params.initialize_from_shape(arg_shapes);

    bool has_srvs = false;
    for (auto const& shape : arg_shapes)
    {
        if (shape.num_srvs > 0)
        {
            has_srvs = true;
            break;
        }
    }

    if (has_srvs)
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

    PR_VK_VERIFY_SUCCESS(vkCreatePipelineLayout(device, &layout_info, nullptr, &raw_layout));
}

void pr::backend::vk::pipeline_layout::initialize(VkDevice device, cc::span<const pr::backend::vk::util::spirv_desc_range_info> range_infos)
{
    detail::pipeline_layout_params params;
    params.initialize_from_reflection_info(range_infos);

    // check for the existence of samplers,
    // and copy pipeline stage visibilities
    bool has_sampler = false;
    for (auto const& range : range_infos)
    {
        if (range.type == VK_DESCRIPTOR_TYPE_SAMPLER)
            has_sampler = true;

        descriptor_set_visibilities.push_back(range.visible_pipeline_stages);
    }

    if (has_sampler)
    {
        // TODO: explicit sampler specification upon shader view creation
        create_implicit_sampler(device);
        params.descriptor_sets[0].fill_in_implicit_sampler(&_implicit_sampler);
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

    PR_VK_VERIFY_SUCCESS(vkCreatePipelineLayout(device, &layout_info, nullptr, &raw_layout));
}

void pr::backend::vk::pipeline_layout::free(VkDevice device)
{
    for (auto const layout : descriptor_set_layouts)
        vkDestroyDescriptorSetLayout(device, layout, nullptr);

    vkDestroyPipelineLayout(device, raw_layout, nullptr);

    if (_implicit_sampler != nullptr)
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

void pr::backend::vk::detail::pipeline_layout_params::initialize_from_shape(pr::backend::arg::shader_argument_shapes arg_shapes)
{
    for (auto const& arg_shape : arg_shapes)
    {
        descriptor_sets.emplace_back();
        descriptor_sets.back().initialize_from_srvs_uavs(arg_shape.num_srvs, arg_shape.num_uavs);
    }

    auto const num_empty_shader_args = limits::max_shader_arguments - arg_shapes.size();
    for (auto _ = 0u; _ < num_empty_shader_args; ++_)
        descriptor_sets.emplace_back();

    for (auto const& arg_shape : arg_shapes)
    {
        descriptor_sets.emplace_back();
        if (arg_shape.has_cb)
            descriptor_sets.back().initialize_from_cbv();
    }
}

void pr::backend::vk::detail::pipeline_layout_params::initialize_from_reflection_info(cc::span<const util::spirv_desc_range_info> range_infos)
{
    descriptor_sets.emplace_back();
    for (auto const& range : range_infos)
    {
        auto const set_delta = range.set - (descriptor_sets.size() - 1);
        if (set_delta == 1)
        {
            // the next set has been reached
            descriptor_sets.emplace_back();
        }
        else if (set_delta > 1)
        {
            // some sets have been skipped
            for (auto i = 0u; i < set_delta; ++i)
                descriptor_sets.emplace_back();
        }

        descriptor_sets.back().add_range(range.type, range.binding_start, range.binding_size, range.visible_stages);
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
        descriptor_sets.push_back(allocator.allocDescriptor(set_layout));
    }
}

void pr::backend::vk::descriptor_set_bundle::free(pr::backend::vk::DescriptorAllocator& allocator)
{
    for (auto desc_set : descriptor_sets)
        allocator.free(desc_set);
}

void pr::backend::vk::descriptor_set_bundle::update_argument(VkDevice device, uint32_t argument_index, const pr::backend::vk::legacy::shader_argument& argument)
{
    auto const& desc_set = descriptor_sets[argument_index];

    cc::capped_vector<VkWriteDescriptorSet, 3> writes;
    VkDescriptorBufferInfo cbv_info;

    if (argument.cbv != nullptr)
    {
        // bind CBV
        auto const& cbv_desc_set = descriptor_sets[argument_index + limits::max_shader_arguments];

        cbv_info = {};
        cbv_info.buffer = argument.cbv;
        cbv_info.offset = argument.cbv_view_offset;
        cbv_info.range = argument.cbv_view_size;

        auto& write = writes.emplace_back();
        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = cbv_desc_set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.descriptorCount = 1; // Just one CBV
        write.pBufferInfo = &cbv_info;
        write.dstArrayElement = 0;
        write.dstBinding = spv::cbv_binding_start; // CBVs always in binding 0
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
        write.dstBinding = spv::uav_binding_start;
    }

    cc::capped_vector<VkDescriptorImageInfo, 8> srv_infos;

    if (!argument.srvs.empty())
    {
        for (auto srv : argument.srvs)
        {
            auto& srv_info = srv_infos.emplace_back();
            srv_info.imageView = srv;
            srv_info.imageLayout = util::to_image_layout(resource_state::shader_resource);
        }

        auto& write = writes.emplace_back();
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = desc_set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        write.descriptorCount = uint32_t(srv_infos.size());
        write.pImageInfo = srv_infos.data();
        write.dstArrayElement = 0;
        write.dstBinding = spv::srv_binding_start;
    }

    vkUpdateDescriptorSets(device, uint32_t(writes.size()), writes.data(), 0, nullptr);
}
