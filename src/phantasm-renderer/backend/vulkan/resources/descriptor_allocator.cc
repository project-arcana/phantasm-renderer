#include "descriptor_allocator.hh"

#include <clean-core/array.hh>
#include <clean-core/assert.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/vulkan/Device.hh>
#include <phantasm-renderer/backend/vulkan/common/native_enum.hh>
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/loader/spirv_patch_util.hh>

namespace pr::backend::vk
{
void DescriptorAllocator::initialize(VkDevice device, uint32_t num_cbvs, uint32_t num_srvs, uint32_t num_uavs, uint32_t num_samplers)
{
    mDevice = device;

    cc::capped_vector<VkDescriptorPoolSize, 6> type_sizes;

    if (num_cbvs > 0)
        type_sizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, num_cbvs});

    if (num_samplers > 0)
        type_sizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, num_samplers});

    if (num_srvs > 0)
    {
        // SRV-only types
        type_sizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, num_srvs});
        type_sizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, num_srvs});
    }

    if (num_uavs > 0)
    {
        // UAV-only types
        type_sizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, num_uavs});
    }

    if (num_srvs + num_uavs > 0)
    {
        // SRV or UAV types
        type_sizes.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, num_srvs + num_uavs});
    }

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = nullptr;
    descriptor_pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool.maxSets = num_srvs + num_uavs + num_cbvs + num_samplers;
    descriptor_pool.poolSizeCount = uint32_t(type_sizes.size());
    descriptor_pool.pPoolSizes = type_sizes.data();

    PR_VK_VERIFY_SUCCESS(vkCreateDescriptorPool(mDevice, &descriptor_pool, nullptr, &mPool));
}

void DescriptorAllocator::destroy() { vkDestroyDescriptorPool(mDevice, mPool, nullptr); }


VkDescriptorSet DescriptorAllocator::allocDescriptor(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.descriptorPool = mPool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout;

    VkDescriptorSet res;
    PR_VK_VERIFY_SUCCESS(vkAllocateDescriptorSets(mDevice, &alloc_info, &res));
    return res;
}

VkDescriptorSet DescriptorAllocator::allocDescriptorFromShape(unsigned num_cbvs, unsigned num_srvs, unsigned num_uavs, VkSampler* immutable_sampler)
{
    auto const layout = createLayoutFromShape(num_cbvs, num_srvs, num_uavs, immutable_sampler);
    auto const res = allocDescriptor(layout);
    vkDestroyDescriptorSetLayout(mDevice, layout, nullptr);
    return res;
}

void DescriptorAllocator::free(VkDescriptorSet descriptor_set) { vkFreeDescriptorSets(mDevice, mPool, 1, &descriptor_set); }

VkDescriptorSetLayout DescriptorAllocator::createLayoutFromShape(unsigned num_cbvs, unsigned num_srvs, unsigned num_uavs, VkSampler* immutable_sampler) const
{
    // NOTE: Eventually arguments could be constrained to stages
    // See pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::add_range
    constexpr auto argument_visibility = VK_SHADER_STAGE_ALL_GRAPHICS;
    cc::capped_vector<VkDescriptorSetLayoutBinding, 4> bindings;

    {
        if (num_cbvs > 0)
        {
            VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
            binding = {};
            binding.binding = spv::cbv_binding_start; // CBV always in (0)
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            binding.descriptorCount = num_cbvs;
            binding.stageFlags = argument_visibility;
            binding.pImmutableSamplers = nullptr; // Optional
        }

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

        if (immutable_sampler != nullptr)
        {
            VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
            binding = {};
            binding.binding = spv::sampler_binding_start;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = argument_visibility;
            binding.pImmutableSamplers = immutable_sampler;
        }
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = uint32_t(bindings.size());
    layout_info.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    PR_VK_VERIFY_SUCCESS(vkCreateDescriptorSetLayout(mDevice, &layout_info, nullptr, &layout));
    return layout;
}

VkDescriptorSetLayout DescriptorAllocator::createLayoutFromShaderViewArgs(cc::span<const shader_view_element> srvs,
                                                                          cc::span<const shader_view_element> uavs,
                                                                          cc::span<VkSampler const> immutable_samplers) const
{
    // NOTE: Eventually arguments could be constrained to stages
    // See pr::backend::vk::detail::pipeline_layout_params::descriptor_set_params::add_range
    constexpr auto argument_visibility = VK_SHADER_STAGE_ALL_GRAPHICS;


    cc::capped_vector<VkDescriptorSetLayoutBinding, 16> bindings;

    {
        if (!srvs.empty())
        {
            VkDescriptorType last_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            auto current_range = 0u;
            auto current_binding_base = spv::srv_binding_start;

            auto const flush_binding = [&]() {
                if (current_range > 0u)
                {
                    VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
                    binding = {};
                    binding.binding = current_binding_base;
                    binding.descriptorCount = current_range;
                    binding.descriptorType = last_type;
                    binding.stageFlags = argument_visibility;
                    binding.pImmutableSamplers = nullptr; // Optional

                    current_binding_base += current_range;
                }

                current_range = 0;
            };

            for (auto const& srv : srvs)
            {
                auto const native_type = util::to_native_srv_desc_type(srv.dimension);
                if (native_type != last_type)
                {
                    flush_binding();
                    last_type = native_type;
                }
                ++current_range;
            }
            flush_binding();
        }

        if (!uavs.empty())
        {
            VkDescriptorType last_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            auto current_range = 0u;
            auto current_binding_base = spv::uav_binding_start;

            auto const flush_binding = [&]() {
                if (current_range > 0u)
                {
                    VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
                    binding = {};
                    binding.binding = current_binding_base;
                    binding.descriptorCount = current_range;
                    binding.descriptorType = last_type;
                    binding.stageFlags = argument_visibility;
                    binding.pImmutableSamplers = nullptr; // Optional

                    current_binding_base += current_range;
                }

                current_range = 0;
            };

            for (auto const& srv : srvs)
            {
                auto const native_type = util::to_native_uav_desc_type(srv.dimension);
                if (native_type != last_type)
                {
                    flush_binding();
                    last_type = native_type;
                }
                ++current_range;
            }
            flush_binding();
        }

        if (!immutable_samplers.empty())
        {
            VkDescriptorSetLayoutBinding& binding = bindings.emplace_back();
            binding = {};
            binding.binding = spv::sampler_binding_start;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
            binding.pImmutableSamplers = immutable_samplers.data();
            binding.descriptorCount = static_cast<unsigned>(immutable_samplers.size());
        }
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = uint32_t(bindings.size());
    layout_info.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    PR_VK_VERIFY_SUCCESS(vkCreateDescriptorSetLayout(mDevice, &layout_info, nullptr, &layout));

    return layout;
}
}
