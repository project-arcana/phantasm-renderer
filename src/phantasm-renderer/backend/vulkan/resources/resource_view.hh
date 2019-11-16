#pragma once

#include <cstdint>

#include <clean-core/span.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
class Device;

class DescriptorAllocator
{
public:
    void initialize(Device* device, uint32_t num_cbvs, uint32_t num_srvs, uint32_t num_uavs, uint32_t num_samplers);
    ~DescriptorAllocator();


    bool allocDescriptor(VkDescriptorSetLayout descriptorLayout, VkDescriptorSet &out_set);
    bool allocDescriptor(int size, const VkSampler* samplers, VkDescriptorSetLayout &out_layout, VkDescriptorSet &out_set);

    bool createDescriptorSetLayoutAndAllocDescriptorSet(cc::span<VkDescriptorSetLayoutBinding> layout_bindings,
                                                        VkDescriptorSetLayout &out_layout,
                                                        VkDescriptorSet &out_set);

    void free(VkDescriptorSet descriptor_set);

private:
    Device* mDevice = nullptr;
    VkDescriptorPool mPool;
    int mNumAllocations = 0;
};

}
