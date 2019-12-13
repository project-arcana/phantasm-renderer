#include "pipeline_layout.hh"

#include <iostream>

#include <phantasm-renderer/backend/vulkan/common/native_enum.hh>
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/resources/descriptor_allocator.hh>
#include <phantasm-renderer/backend/vulkan/resources/transition_barrier.hh>

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::add_single_cbv()
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

void pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::add_descriptor(VkDescriptorType type, unsigned binding, unsigned array_size, VkShaderStageFlagBits visibility)
{
    VkDescriptorSetLayoutBinding& new_binding = bindings.emplace_back();
    new_binding = {};
    new_binding.binding = binding;
    new_binding.descriptorType = type;
    new_binding.descriptorCount = array_size;

    // TODO: We have access to precise visibility constraints _in this function_, in the `visibility` argument
    // however, in shader_view_pool, DescriptorSets must be created without this knowledge, which is why
    // the pool falls back to VK_SHADER_STAGE_ALL_GRAPHICS for its temporary layouts. And since the descriptors would
    // be incompatible, we have to use the same thing here
    new_binding.stageFlags = (visibility == VK_SHADER_STAGE_COMPUTE_BIT) ? visibility : VK_SHADER_STAGE_ALL_GRAPHICS;
    new_binding.pImmutableSamplers = nullptr; // Optional
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

void pr::backend::vk::pipeline_layout::initialize(VkDevice device, cc::span<const util::spirv_desc_info> range_infos)
{
    detail::pipeline_layout_params params;
    params.initialize_from_reflection_info(range_infos);

    // copy pipeline stage visibilities
    for (auto const& range : range_infos)
    {
        descriptor_set_visibilities.push_back(range.visible_pipeline_stage);
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
}

void pr::backend::vk::detail::pipeline_layout_params::initialize_from_reflection_info(cc::span<const util::spirv_desc_info> reflection_info)
{
    descriptor_sets.emplace_back();
    for (auto const& desc : reflection_info)
    {
        auto const set_delta = desc.set - (descriptor_sets.size() - 1);
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

        descriptor_sets.back().add_descriptor(desc.type, desc.binding, desc.binding_array_size, desc.visible_stage);
    }
}
