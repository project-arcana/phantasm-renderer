#include "pipeline_layout.hh"

#include <phantasm-renderer/backend/vulkan/common/native_enum.hh>
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/resources/transition_barrier.hh>
#include <phantasm-renderer/backend/vulkan/resources/descriptor_allocator.hh>

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

    // TODO: We have access to precise visibility constraints _in this function_, in the `visibility` argument
    // however, in shader_view_pool, DescriptorSets must be created without this knowledge, which is why
    // the pool falls back to VK_SHADER_STAGE_ALL_GRAPHICS for its temporary layouts. And since the descriptors would
    // be incompatible, we have to use the same thing here
    binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    binding.pImmutableSamplers = nullptr; // Optional
}

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::fill_in_samplers(cc::span<VkSampler const> samplers)
{
    for (auto& binding : bindings)
    {
        if (binding.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER)
        {
            binding.pImmutableSamplers = samplers.data();
            return;
        }
    }

    CC_ASSERT(false && "Failed to fill in samplers - not present in shader");
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

void pr::backend::vk::pipeline_layout::initialize(VkDevice device,
                                                  cc::span<const pr::backend::vk::util::spirv_desc_range_info> range_infos,
                                                  arg::shader_sampler_configs samplers,
                                                  DescriptorAllocator& desc_allocator)
{
    detail::pipeline_layout_params params;
    params.initialize_from_reflection_info(range_infos);

    // copy pipeline stage visibilities
    for (auto const& range : range_infos)
    {
        descriptor_set_visibilities.push_back(range.visible_pipeline_stages);
    }

    for (auto const& sampler_config : samplers)
    {
        create_sampler(device, sampler_config);
    }

    if (!_samplers.empty())
    {
        params.descriptor_sets[spv::static_sampler_descriptor_set].fill_in_samplers(_samplers);
    }

    for (auto const& param_set : params.descriptor_sets)
    {
        descriptor_set_layouts.push_back(param_set.create_layout(device));
    }

    if (!_samplers.empty())
    {
        _sampler_descriptor_set = desc_allocator.allocDescriptor(descriptor_set_layouts[spv::static_sampler_descriptor_set]);

        // Perform the initial update to this descriptor set
        //        cc::capped_vector<VkDescriptorImageInfo, limits::max_shader_samplers> image_infos;
        //        {
        //            for (auto const sampler : _samplers)
        //            {
        //                auto& info = image_infos.emplace_back();
        //                info = {};
        //                info.sampler = sampler;
        //                info.imageView = nullptr;
        //                info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        //            }
        //        }

        //        VkWriteDescriptorSet write = {};
        //        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //        write.pNext = nullptr;
        //        write.dstSet = _sampler_descriptor_set;
        //        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        //        write.dstArrayElement = 0;
        //        write.dstBinding = spv::sampler_binding_start;
        //        write.descriptorCount = static_cast<uint32_t>(samplers.size());

        //        write.pImageInfo = image_infos.data();

        //        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
    else
    {
        _sampler_descriptor_set = nullptr;
    }

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = uint32_t(descriptor_set_layouts.size());
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    layout_info.pushConstantRangeCount = 0;
    layout_info.pPushConstantRanges = nullptr;

    PR_VK_VERIFY_SUCCESS(vkCreatePipelineLayout(device, &layout_info, nullptr, &raw_layout));
}

void pr::backend::vk::pipeline_layout::free(VkDevice device, DescriptorAllocator& desc_allocator)
{
    if (_sampler_descriptor_set != nullptr)
    {
        desc_allocator.free(_sampler_descriptor_set);
    }

    for (auto const layout : descriptor_set_layouts)
        vkDestroyDescriptorSetLayout(device, layout, nullptr);

    vkDestroyPipelineLayout(device, raw_layout, nullptr);

    for (auto const s : _samplers)
    {
        vkDestroySampler(device, s, nullptr);
    }
}

void pr::backend::vk::pipeline_layout::create_sampler(VkDevice device, const sampler_config& config)
{
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.minFilter = util::to_min_filter(config.filter);
    info.magFilter = util::to_mag_filter(config.filter);
    info.mipmapMode = util::to_mipmap_filter(config.filter);
    info.addressModeU = util::to_native(config.address_u);
    info.addressModeV = util::to_native(config.address_v);
    info.addressModeW = util::to_native(config.address_w);
    info.minLod = config.min_lod;
    info.maxLod = config.max_lod;
    info.mipLodBias = config.lod_bias;
    info.anisotropyEnable = config.filter == sampler_filter::anisotropic ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy = static_cast<float>(config.max_anisotropy);
    info.borderColor = util::to_native(config.border_color);
    info.compareEnable = config.compare_func != sampler_compare_func::disabled ? VK_TRUE : VK_FALSE;
    info.compareOp = util::to_native(config.compare_func);

    auto& new_sampler = _samplers.emplace_back();
    PR_VK_VERIFY_SUCCESS(vkCreateSampler(device, &info, nullptr, &new_sampler));
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
