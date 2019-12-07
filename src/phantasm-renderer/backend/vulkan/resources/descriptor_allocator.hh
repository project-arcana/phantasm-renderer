#pragma once

#include <cstdint>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

#include <phantasm-renderer/backend/arguments.hh>

namespace pr::backend::vk
{
/// Unsynchronized
class DescriptorAllocator
{
public:
    void initialize(VkDevice device, uint32_t num_cbvs, uint32_t num_srvs, uint32_t num_uavs, uint32_t num_samplers);
    void destroy();

    // requires sync
    [[nodiscard]] VkDescriptorSet allocDescriptor(VkDescriptorSetLayout descriptorLayout);

    // requires sync
    [[nodiscard]] VkDescriptorSet allocDescriptorFromShape(unsigned num_cbvs, unsigned num_srvs, unsigned num_uavs, VkSampler* immutable_sampler);

    // requires sync
    void free(VkDescriptorSet descriptor_set);

    // free-threaded
    [[nodiscard]] VkDescriptorSetLayout createLayoutFromShape(unsigned num_cbvs, unsigned num_srvs, unsigned num_uavs, VkSampler* immutable_sampler) const;

    [[nodiscard]] VkDescriptorSetLayout createLayoutFromShaderViewArgs(cc::span<shader_view_element const> srvs,
                                                                       cc::span<shader_view_element const> uavs,
                                                                       cc::span<VkSampler const> immutable_samplers) const;


    [[nodiscard]] VkDevice getDevice() const { return mDevice; }

private:
    VkDevice mDevice = nullptr;
    VkDescriptorPool mPool;
};

}
