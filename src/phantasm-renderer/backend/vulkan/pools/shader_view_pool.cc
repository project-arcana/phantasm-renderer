#include "shader_view_pool.hh"

#include <iostream>

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/vulkan/loader/spirv_patch_util.hh>
#include <phantasm-renderer/backend/vulkan/resources/resource_state.hh>

pr::backend::handle::shader_view pr::backend::vk::ShaderViewPool::create(cc::span<pr::backend::handle::resource> srvs, cc::span<pr::backend::handle::resource> uavs)
{
    VkDescriptorSet res_raw;
    unsigned pool_index;

    // Create the layout
    auto const layout = mAllocator.createLayoutFromShape(0, srvs.size(), uavs.size(), nullptr);

    // Do acquires requiring synchronization first
    {
        auto lg = std::lock_guard(mMutex);
        res_raw = mAllocator.allocDescriptor(layout);
        pool_index = mPool.acquire();
    }

    // Clean up the layout
    vkDestroyDescriptorSetLayout(mAllocator.getDevice(), layout, nullptr);

    // Populate new node
    shader_view_node& new_node = mPool.get(pool_index);
    new_node.raw_desc_set = res_raw;

    // Perform the writes
    {
        cc::capped_vector<VkWriteDescriptorSet, 2> writes;
        cc::capped_vector<VkDescriptorBufferInfo, 8> uav_infos;
        cc::capped_vector<VkDescriptorImageInfo, 8> srv_infos;

        if (!uavs.empty())
        {
            for (auto uav : uavs)
            {
                auto& uav_info = uav_infos.emplace_back();
                CC_RUNTIME_ASSERT(false && "TODO");
//                uav_info.buffer = uav.buffer;
//                uav_info.offset = uav.offset;
//                uav_info.range = uav.range;
            }

            auto& write = writes.emplace_back();
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.pNext = nullptr;
            write.dstSet = res_raw;
            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            write.descriptorCount = uint32_t(uav_infos.size());
            write.pBufferInfo = uav_infos.data();
            write.dstArrayElement = 0;
            write.dstBinding = spv::uav_binding_start;
        }

        if (!srvs.empty())
        {
            for (auto srv : srvs)
            {
                auto& srv_info = srv_infos.emplace_back();
                CC_RUNTIME_ASSERT(false && "TODO");
//                srv_info.imageView = srv;
                srv_info.imageLayout = to_image_layout(resource_state::shader_resource);
            }

            auto& write = writes.emplace_back();
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.pNext = nullptr;
            write.dstSet = res_raw;
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.descriptorCount = uint32_t(srv_infos.size());
            write.pImageInfo = srv_infos.data();
            write.dstArrayElement = 0;
            write.dstBinding = spv::srv_binding_start;
        }

        vkUpdateDescriptorSets(mAllocator.getDevice(), uint32_t(writes.size()), writes.data(), 0, nullptr);
    }

    return {static_cast<handle::index_t>(pool_index)};
}

void pr::backend::vk::ShaderViewPool::free(pr::backend::handle::shader_view sv)
{
    // TODO: dangle check

    shader_view_node& freed_node = mPool.get(static_cast<unsigned>(sv.index));

    {
        // This is a write access to the pool and allocator, and must be synced
        auto lg = std::lock_guard(mMutex);
        mAllocator.free(freed_node.raw_desc_set);
        mPool.release(static_cast<unsigned>(sv.index));
    }
}

void pr::backend::vk::ShaderViewPool::initialize(VkDevice device, ResourcePool* res_pool, int num_cbvs, int num_srvs, int num_uavs, int num_samplers)
{
    mResourcePool = res_pool;

    mAllocator.initialize(device, num_cbvs, num_srvs, num_uavs, num_samplers);
    // Due to the fact that each shader argument represents up to one CBV, this is the upper limit for the amount of shader_view handles
    mPool.initialize(num_cbvs);
}

void pr::backend::vk::ShaderViewPool::destroy()
{
    auto num_leaks = 0;
    mPool.iterate_allocated_nodes([&](shader_view_node& leaked_node) {
        ++num_leaks;
        mAllocator.free(leaked_node.raw_desc_set);
    });

    if (num_leaks > 0)
    {
        std::cout << "[pr][backend][vk] warning: leaked " << num_leaks << " handle::shader_view object" << (num_leaks == 1 ? "" : "s") << std::endl;
    }

    mAllocator.destroy();
}
