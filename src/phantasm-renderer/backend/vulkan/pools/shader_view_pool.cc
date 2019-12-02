#include "shader_view_pool.hh"

#include <iostream>

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/vulkan/common/native_enum.hh>
#include <phantasm-renderer/backend/vulkan/common/vk_format.hh>
#include <phantasm-renderer/backend/vulkan/loader/spirv_patch_util.hh>
#include <phantasm-renderer/backend/vulkan/resources/resource_state.hh>

#include "resource_pool.hh"

pr::backend::handle::shader_view pr::backend::vk::ShaderViewPool::create(cc::span<shader_view_element const> srvs, cc::span<shader_view_element const> uavs)
{
    VkDescriptorSet res_raw;
    unsigned pool_index;

    // Create the layout, maps as follows:
    // SRV:
    //      Texture* -> VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    //      RT AS    -> VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV
    //      Buffer   -> VK_DESCRIPTOR_TYPE_STORAGE_BUFFER (or UNIFORM_BUFFER, STORAGE_TEXEL_BUFFER? This one is ambiguous)
    // UAV:
    //      Texture* -> VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
    //      Buffer   -> VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    auto const layout = mAllocator.createLayoutFromShaderViewArgs(srvs, uavs, {});

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
        cc::capped_vector<VkWriteDescriptorSet, 24> writes;
        cc::capped_vector<VkDescriptorBufferInfo, 16> buffer_infos;
        cc::capped_vector<VkDescriptorImageInfo, 16> image_infos;

        VkDescriptorType last_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
        auto current_buffer_range = 0u;
        auto current_image_range = 0u;
        auto current_binding_base = 0u;

        auto const flush_writes = [&]() {
            if (current_buffer_range + current_image_range > 0u)
            {
                auto& write = writes.emplace_back();
                write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.pNext = nullptr;
                write.dstSet = res_raw;
                write.descriptorType = last_type;
                write.dstArrayElement = 0;
                write.dstBinding = current_binding_base;

                if (current_buffer_range > 0)
                {
                    write.descriptorCount = current_buffer_range;
                    write.pBufferInfo = buffer_infos.data() + (buffer_infos.size() - current_buffer_range);
                    current_buffer_range = 0;
                }
                else
                {
                    // current_image_range > 0
                    write.descriptorCount = current_image_range;
                    write.pImageInfo = image_infos.data() + (image_infos.size() - current_image_range);
                    current_image_range = 0;
                }


                current_binding_base += write.descriptorCount;
            }
        };

        current_binding_base = spv::uav_binding_start;
        for (auto const& uav : uavs)
        {
            auto const uav_native_type = util::to_native_uav_desc_type(uav.dimension);
            if (last_type != uav_native_type)
            {
                flush_writes();
                last_type = uav_native_type;
            }

            if (uav.dimension == shader_view_dimension::buffer)
            {
                auto& uav_info = buffer_infos.emplace_back();
                uav_info.buffer = mResourcePool->getRawBuffer(uav.resource);
                uav_info.offset = uav.buffer_info.element_start;
                uav_info.range = uav.buffer_info.element_size;

                ++current_buffer_range;
            }
            else
            {
                // shader_view_dimension::textureX
                CC_ASSERT(uav.dimension != shader_view_dimension::raytracing_accel_struct && "Raytracing acceleration structures not allowed as UAVs");

                auto& img_info = image_infos.emplace_back();
                img_info.imageView = makeImageView(uav);
                img_info.imageLayout = util::to_image_layout(mResourcePool->getResourceState(uav.resource));
                img_info.sampler = nullptr;

                ++current_image_range;
            }
        }

        flush_writes();

        current_binding_base = spv::srv_binding_start;
        for (auto const& srv : srvs)
        {
            auto const uav_native_type = util::to_native_srv_desc_type(srv.dimension);
            if (last_type != uav_native_type)
            {
                flush_writes();
                last_type = uav_native_type;
            }

            if (srv.dimension == shader_view_dimension::buffer)
            {
                auto& uav_info = buffer_infos.emplace_back();
                uav_info.buffer = mResourcePool->getRawBuffer(srv.resource);
                uav_info.offset = srv.buffer_info.element_start;
                uav_info.range = srv.buffer_info.element_size;

                ++current_buffer_range;
            }
            else if (srv.dimension == shader_view_dimension::raytracing_accel_struct)
            {
                CC_RUNTIME_ASSERT(false && "Unimplemented!");

                VkWriteDescriptorSetAccelerationStructureNV as_info = {};
                as_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV;
                as_info.accelerationStructureCount = 1;
                as_info.pAccelerationStructures = nullptr; // TODO: Retrieve from res pool

                auto& write = writes.emplace_back();
                write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.pNext = &as_info; // TODO: keep as_info alive
                write.dstSet = res_raw;
                write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
                write.dstArrayElement = 0;
                write.dstBinding = current_binding_base;
                write.descriptorCount = 1;

                current_binding_base += write.descriptorCount;
            }
            else
            {
                // shader_view_dimension::textureX

                auto& img_info = image_infos.emplace_back();
                img_info.imageView = makeImageView(srv);
                img_info.imageLayout = util::to_image_layout(mResourcePool->getResourceState(srv.resource));
                img_info.sampler = nullptr;

                ++current_image_range;
            }
        }

        flush_writes();

        vkUpdateDescriptorSets(mAllocator.getDevice(), uint32_t(writes.size()), writes.data(), 0, nullptr);

        for (auto const& img_info : image_infos)
        {
            // NOTE: Maybe the lifetime of image views is required to be longer
            // in that case, store them in the pool node as well
            vkDestroyImageView(mDevice, img_info.imageView, nullptr);
        }
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
    mDevice = device;
    mResourcePool = res_pool;

    mAllocator.initialize(mDevice, num_cbvs, num_srvs, num_uavs, num_samplers);
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

VkImageView pr::backend::vk::ShaderViewPool::makeImageView(const shader_view_element& sve) const
{
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = mResourcePool->getRawImage(sve.resource);
    info.viewType = util::to_native_image_view_type(sve.dimension);
    info.format = util::to_vk_format(sve.pixel_format);
    if (is_depth_format(sve.pixel_format))
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    info.subresourceRange.baseMipLevel = sve.texture_info.mip_start;
    info.subresourceRange.levelCount = sve.texture_info.mip_size;
    info.subresourceRange.baseArrayLayer = sve.texture_info.array_start;
    info.subresourceRange.layerCount = sve.texture_info.array_size;

    VkImageView res;
    auto const vr = vkCreateImageView(mDevice, &info, nullptr, &res);
    CC_ASSERT(vr == VK_SUCCESS);
    return res;
}
