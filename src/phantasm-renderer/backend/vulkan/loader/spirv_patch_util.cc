#include "spirv_patch_util.hh"

#include <algorithm>

#include <clean-core/array.hh>
#include <clean-core/bit_cast.hh>
#include <clean-core/utility.hh>

#include <phantasm-renderer/backend/detail/unique_buffer.hh>
#include <phantasm-renderer/backend/lib/spirv_reflect.hh>
#include <phantasm-renderer/backend/limits.hh>

#include <phantasm-renderer/backend/vulkan/common/native_enum.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace
{
[[nodiscard]] constexpr VkDescriptorType reflect_to_native(SpvReflectDescriptorType type)
{
    switch (type)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    }
}

[[nodiscard]] constexpr pr::backend::shader_domain reflect_to_pr(SpvReflectShaderStageFlagBits shader_stage_flags)
{
    using sd = pr::backend::shader_domain;
    switch (shader_stage_flags)
    {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
        return sd::vertex;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return sd::hull;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return sd::domain;
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
        return sd::geometry;
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
        return sd::pixel;
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
        return sd::compute;
    }
}

void patchSpvReflectShader(SpvReflectShaderModule& module, pr::backend::shader_domain current_stage, cc::vector<pr::backend::vk::util::spirv_desc_info>& out_desc_infos)
{
    using namespace pr::backend::vk;

    // shift CBVs up by [max_shader_arguments] sets
    {
        uint32_t num_bindings;
        spvReflectEnumerateDescriptorBindings(&module, &num_bindings, nullptr);
        cc::array<SpvReflectDescriptorBinding*> bindings(num_bindings);
        spvReflectEnumerateDescriptorBindings(&module, &num_bindings, bindings.data());

        for (auto b : bindings)
        {
            if (b->resource_type == SPV_REFLECT_RESOURCE_FLAG_CBV)
            {
                auto const new_set = b->set + pr::backend::limits::max_shader_arguments;
                spvReflectChangeDescriptorBindingNumbers(&module, b, b->binding, new_set);
            }
        }
    }

    // push the found descriptors
    {
        uint32_t num_bindings;
        spvReflectEnumerateDescriptorBindings(&module, &num_bindings, nullptr);
        cc::array<SpvReflectDescriptorBinding*> bindings(num_bindings);
        spvReflectEnumerateDescriptorBindings(&module, &num_bindings, bindings.data());

        out_desc_infos.reserve(out_desc_infos.size() + num_bindings);

        for (auto b : bindings)
        {
            auto& new_info = out_desc_infos.emplace_back();
            new_info.set = b->set;
            new_info.binding = b->binding;
            new_info.binding_array_size = b->count;
            new_info.type = reflect_to_native(b->descriptor_type);
            new_info.visible_stage = util::to_shader_stage_flags(current_stage);
            new_info.visible_pipeline_stage = util::to_pipeline_stage_flags(current_stage);
        }
    }

    return;
}

}

pr::backend::arg::shader_stage pr::backend::vk::util::create_patched_spirv(std::byte const* bytecode, size_t bytecode_size, cc::vector<spirv_desc_info>& out_desc_infos)
{
    arg::shader_stage res;

    SpvReflectShaderModule module;
    spvReflectCreateShaderModule(bytecode_size, bytecode, &module);

    res.domain = reflect_to_pr(module.shader_stage);
    patchSpvReflectShader(module, res.domain, out_desc_infos);

    res.binary_size = spvReflectGetCodeSize(&module);
    res.binary_data = cc::bit_cast<std::byte*>(module._internal->spirv_code);

    // spirv-reflect internally checks if this field is a nullptr before calling ::free,
    // so we can keep it alive by setting this
    module._internal->spirv_code = nullptr;

    spvReflectDestroyShaderModule(&module);
    return res;
}


void pr::backend::vk::util::free_patched_spirv(const arg::shader_stage& val)
{
    // do the same thing spirv-reflect would have done in spvReflectDestroyShaderModule
    ::free(val.binary_data);
}

pr::backend::arg::shader_stage pr::backend::vk::util::create_patched_spirv_from_binary_file(const char* filename, cc::vector<spirv_desc_info>& out_desc_infos)
{
    auto const binary_data = detail::unique_buffer::create_from_binary_file(filename);
    CC_RUNTIME_ASSERT(binary_data.is_valid() && "Could not open SPIR-V binary");
    return create_patched_spirv(binary_data.get(), binary_data.size(), out_desc_infos);
}

cc::vector<pr::backend::vk::util::spirv_desc_info> pr::backend::vk::util::merge_spirv_descriptors(cc::span<spirv_desc_info> desc_infos)
{
    // sort by set, then binding (both ascending)
    std::sort(desc_infos.begin(), desc_infos.end(), [](spirv_desc_info const& lhs, spirv_desc_info const& rhs) {
        if (lhs.set != rhs.set)
            return lhs.set < rhs.set;
        else
            return lhs.binding < rhs.binding;
    });

    cc::vector<spirv_desc_info> sorted_merged_res;
    sorted_merged_res.reserve(desc_infos.size());
    spirv_desc_info* curr_range = nullptr;

    for (auto& di : desc_infos)
    {
        if (curr_range &&                     // not the first range
            curr_range->set == di.set &&      // set same as current range
            curr_range->binding == di.binding // binding same as current range
        )
        {
            CC_ASSERT(curr_range->type == di.type && "SPIR-V descriptor type overlap detected");
            CC_ASSERT(curr_range->binding_array_size == di.binding_array_size && "SPIR-V descriptor array mismatch detected");

            // this element mirrors the precursor, bit-OR the shader stage bits
            curr_range->visible_stage = static_cast<VkShaderStageFlagBits>(curr_range->visible_stage | di.visible_stage);
            curr_range->visible_pipeline_stage = static_cast<VkPipelineStageFlags>(curr_range->visible_pipeline_stage | di.visible_pipeline_stage);
        }
        else
        {
            sorted_merged_res.push_back(di);
            curr_range = &sorted_merged_res.back();
        }
    }

    // change the CBVs to UNIFORM_BUFFER_DYNAMIC
    for (auto& range : sorted_merged_res)
    {
        if (range.set >= limits::max_shader_arguments && range.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            range.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    }

    return sorted_merged_res;
}

bool pr::backend::vk::util::is_consistent_with_reflection(cc::span<const pr::backend::vk::util::spirv_desc_info> spirv_ranges,
                                                          pr::backend::arg::shader_argument_shapes arg_shapes)
{
    struct reflected_range_infos
    {
        unsigned num_cbvs = 0;
        unsigned num_srvs = 0;
        unsigned num_uavs = 0;
        unsigned num_samplers = 0;
    };

    cc::array<reflected_range_infos, limits::max_shader_arguments> range_infos;

    for (auto const& range : spirv_ranges)
    {
        auto set_shape_index = range.set;
        if (set_shape_index >= limits::max_shader_arguments)
            set_shape_index -= limits::max_shader_arguments;

        reflected_range_infos& info = range_infos[set_shape_index];

        if (range.binding >= spv::sampler_binding_start)
        {
            info.num_samplers = cc::max(info.num_samplers, 1 + (range.binding - spv::sampler_binding_start));
        }
        else if (range.binding >= spv::uav_binding_start)
        {
            info.num_uavs = cc::max(info.num_uavs, 1 + (range.binding - spv::uav_binding_start));
        }
        else if (range.binding >= spv::srv_binding_start)
        {
            info.num_srvs = cc::max(info.num_srvs, 1 + (range.binding - spv::srv_binding_start));
        }
        else /*if (range.binding >= spv::cbv_binding_start)*/
        {
            info.num_cbvs = cc::max(info.num_cbvs, 1 + (range.binding - spv::cbv_binding_start));
        }
    }

    auto const is_matching = [](reflected_range_infos const& ri, arg::shader_argument_shape const& shape) {
        return (ri.num_cbvs == (shape.has_cb ? 1 : 0)) && (ri.num_srvs == shape.num_srvs) && (ri.num_uavs == shape.num_uavs)
               && (ri.num_samplers == shape.num_samplers);
    };

    for (auto i = 0u; i < arg_shapes.size(); ++i)
    {
        if (!is_matching(range_infos[i], arg_shapes[i]))
            return false;
    }
    return true;
}
