#include "resource_view.hh"

#include <clean-core/array.hh>
#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/vulkan/Device.hh>

namespace pr::backend::vk
{
void DescriptorAllocator::initialize(Device* device, uint32_t num_cbvs, uint32_t num_srvs, uint32_t num_uavs, uint32_t num_samplers)
{
    mDevice = device;
    mNumAllocations = 0;

    cc::array const type_count
        = {VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, num_cbvs}, VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, num_srvs},
           VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, num_samplers}, VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, num_uavs}};

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = nullptr;
    descriptor_pool.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool.maxSets = 6000;
    descriptor_pool.poolSizeCount = uint32_t(type_count.size());
    descriptor_pool.pPoolSizes = type_count.data();

    VkResult res = vkCreateDescriptorPool(mDevice->getDevice(), &descriptor_pool, nullptr, &mPool);
    CC_ASSERT(res == VK_SUCCESS);
}

DescriptorAllocator::~DescriptorAllocator()
{
    if (mDevice)
    {
        vkDestroyDescriptorPool(mDevice->getDevice(), mPool, nullptr);
    }
}

bool DescriptorAllocator::createDescriptorSetLayoutAndAllocDescriptorSet(cc::span<VkDescriptorSetLayoutBinding> layout_bindings,
                                                                         VkDescriptorSetLayout& out_layout,
                                                                         VkDescriptorSet& out_set)
{
    // Next take layout bindings and use them to create a descriptor set layout

    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = nullptr;
    descriptor_layout.bindingCount = uint32_t(layout_bindings.size());
    descriptor_layout.pBindings = layout_bindings.data();

    VkResult res = vkCreateDescriptorSetLayout(mDevice->getDevice(), &descriptor_layout, nullptr, &out_layout);
    CC_ASSERT(res == VK_SUCCESS);

    return allocDescriptor(out_layout, out_set);
}

bool DescriptorAllocator::allocDescriptor(VkDescriptorSetLayout layout, VkDescriptorSet& out_set)
{
    // std::lock_guard<std::mutex> lock(m_mutex);

    VkDescriptorSetAllocateInfo alloc_info;
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.descriptorPool = mPool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout;

    VkResult res = vkAllocateDescriptorSets(mDevice->getDevice(), &alloc_info, &out_set);
    CC_ASSERT(res == VK_SUCCESS);

    mNumAllocations++;

    return res == VK_SUCCESS;
}

void DescriptorAllocator::free(VkDescriptorSet descriptor_set)
{
    mNumAllocations--;
    vkFreeDescriptorSets(mDevice->getDevice(), mPool, 1, &descriptor_set);
}

bool DescriptorAllocator::allocDescriptor(int size, const VkSampler* samplers, VkDescriptorSetLayout& out_layout, VkDescriptorSet& out_set)
{
    auto layoutBindings = cc::array<VkDescriptorSetLayoutBinding>::uninitialized(size_t(size));

    for (auto i = 0u; i < unsigned(size); ++i)
    {
        layoutBindings[i].binding = i;
        layoutBindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[i].descriptorCount = 1;
        layoutBindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[i].pImmutableSamplers = (samplers != nullptr) ? &samplers[i] : nullptr;
    }

    return createDescriptorSetLayoutAndAllocDescriptorSet(layoutBindings, out_layout, out_set);
}
}
