#include "resource_pool.hh"

#include <iostream>

#include <clean-core/bit_cast.hh>
#include <clean-core/utility.hh>

#include <typed-geometry/tg.hh>

#include <phantasm-renderer/backend/vulkan/common/native_enum.hh>
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/common/vk_format.hh>
#include <phantasm-renderer/backend/vulkan/loader/spirv_patch_util.hh>
#include <phantasm-renderer/backend/vulkan/memory/VMA.hh>

namespace
{
unsigned calculate_num_mip_levels(unsigned width, unsigned height)
{
    return static_cast<unsigned>(tg::floor(tg::log2(static_cast<float>(cc::max(width, height))))) + 1u;
}
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::createTexture(
    format format, unsigned w, unsigned h, unsigned mips, texture_dimension dim, unsigned depth_or_array_size, bool allow_uav)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;

    image_info.imageType = util::to_native(dim);
    image_info.format = util::to_vk_format(format);

    image_info.extent.width = uint32_t(w);
    image_info.extent.height = uint32_t(h);
    image_info.extent.depth = dim == texture_dimension::t3d ? depth_or_array_size : 1;
    image_info.mipLevels = mips < 1 ? calculate_num_mip_levels(w, h) : uint32_t(mips);
    image_info.arrayLayers = dim == texture_dimension::t3d ? 1 : depth_or_array_size;

    image_info.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: Configurable
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = nullptr;

    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (allow_uav)
    {
        // TODO: Image usage transfer src might deserve its own option, this is coarse
        // in fact we might want to create a pr::texture_usage enum
        image_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    image_info.flags = 0;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

    if (dim == texture_dimension::t2d && depth_or_array_size == 6)
    {
        image_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VmaAllocation res_alloc;
    VkImage res_image;
    PR_VK_VERIFY_SUCCESS(vmaCreateImage(mAllocator.getAllocator(), &image_info, &alloc_info, &res_image, &res_alloc, nullptr));
    return acquireImage(res_alloc, res_image, format, resource_state::undefined, image_info.mipLevels, image_info.arrayLayers);
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::createRenderTarget(pr::backend::format format, unsigned w, unsigned h, unsigned samples)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = util::to_vk_format(format);
    image_info.extent.width = static_cast<uint32_t>(w);
    image_info.extent.height = static_cast<uint32_t>(h);
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = util::to_native_sample_flags(samples);

    // only undefined or preinitialized are legal options
    image_info.initialLayout = util::to_image_layout(resource_state::undefined);

    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = nullptr;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    if (backend::is_depth_format(format))
        image_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    else
        image_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    image_info.flags = 0;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VmaAllocation res_alloc;
    VkImage res_image;
    PR_VK_VERIFY_SUCCESS(vmaCreateImage(mAllocator.getAllocator(), &image_info, &alloc_info, &res_image, &res_alloc, nullptr));
    return acquireImage(res_alloc, res_image, format, resource_state::undefined, image_info.mipLevels, image_info.arrayLayers);
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::createBuffer(unsigned size_bytes, pr::backend::resource_state initial_state, unsigned stride_bytes)
{
    // TODO: we ignore the initial state here entirely
    (void)initial_state;

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size_bytes;
    // right now we'll just take all usages this thing might have in API semantics
    // it might be required down the line to restrict this (as in, make it part of API)
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                        | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VmaAllocation res_alloc;
    VkBuffer res_buffer;
    PR_VK_VERIFY_SUCCESS(vmaCreateBuffer(mAllocator.getAllocator(), &buffer_info, &alloc_info, &res_buffer, &res_alloc, nullptr));
    return acquireBuffer(res_alloc, res_buffer, resource_state::undefined, size_bytes, stride_bytes);
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::createMappedBuffer(unsigned size_bytes, unsigned stride_bytes)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size_bytes;
    // right now we'll just take all usages this thing might have in API semantics
    // it might be required down the line to restrict this (as in, make it part of API)
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                        | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc_info.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocation res_alloc;
    VmaAllocationInfo res_alloc_info;
    VkBuffer res_buffer;
    PR_VK_VERIFY_SUCCESS(vmaCreateBuffer(mAllocator.getAllocator(), &buffer_info, &alloc_info, &res_buffer, &res_alloc, &res_alloc_info));
    CC_ASSERT(res_alloc_info.pMappedData != nullptr);
    return acquireBuffer(res_alloc, res_buffer, resource_state::undefined, size_bytes, stride_bytes, cc::bit_cast<std::byte*>(res_alloc_info.pMappedData));
}

void pr::backend::vk::ResourcePool::free(pr::backend::handle::resource res)
{
    CC_ASSERT(res != mInjectedBackbufferResource && "the backbuffer resource must not be freed");

    resource_node& freed_node = mPool.get(static_cast<unsigned>(res.index));

    {
        auto lg = std::lock_guard(mMutex);
        // This is a write access to mAllocatorDescriptors
        internalFree(freed_node);
        // This is a write access to the pool and must be synced
        mPool.release(static_cast<unsigned>(res.index));
    }
}

void pr::backend::vk::ResourcePool::free(cc::span<const pr::backend::handle::resource> resources)
{
    auto lg = std::lock_guard(mMutex);

    for (auto res : resources)
    {
        CC_ASSERT(res != mInjectedBackbufferResource && "the backbuffer resource must not be freed");
        if (res.is_valid())
        {
            resource_node& freed_node = mPool.get(static_cast<unsigned>(res.index));
            // This is a write access to mAllocatorDescriptors
            internalFree(freed_node);
            // This is a write access to the pool and must be synced
            mPool.release(static_cast<unsigned>(res.index));
        }
    }
}

void pr::backend::vk::ResourcePool::initialize(VkPhysicalDevice physical, VkDevice device, unsigned max_num_resources)
{
    mAllocator.initialize(physical, device);
    mAllocatorDescriptors.initialize(device, max_num_resources, 0, 0, 0);
    mPool.initialize(max_num_resources + 1); // 1 additional resource for the backbuffer
    mInjectedBackbufferResource = {static_cast<handle::index_t>(mPool.acquire())};
}

void pr::backend::vk::ResourcePool::destroy()
{
    auto num_leaks = 0;
    mPool.iterate_allocated_nodes([&](resource_node& leaked_node) {
        if (leaked_node.allocation != nullptr)
        {
            ++num_leaks;
            internalFree(leaked_node);
        }
    });

    if (num_leaks > 0)
    {
        std::cout << "[pr][backend][vk] warning: leaked " << num_leaks << " handle::resource object" << (num_leaks == 1 ? "" : "s") << std::endl;
    }

    mAllocator.destroy();
    mAllocatorDescriptors.destroy();
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::injectBackbufferResource(VkImage raw_image, pr::backend::resource_state state, VkImageView backbuffer_view)
{
    resource_node& backbuffer_node = mPool.get(static_cast<unsigned>(mInjectedBackbufferResource.index));
    backbuffer_node.type = resource_node::resource_type::image;
    backbuffer_node.image.raw_image = raw_image;
    backbuffer_node.master_state = state;
    backbuffer_node.master_state_dependency = util::to_pipeline_stage_dependency(state, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);
    mInjectedBackbufferView = backbuffer_view;
    return mInjectedBackbufferResource;
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::acquireBuffer(
    VmaAllocation alloc, VkBuffer buffer, pr::backend::resource_state initial_state, unsigned buffer_width, unsigned buffer_stride, std::byte* buffer_map)
{
    unsigned res;
    VkDescriptorSetLayout cbv_desc_set_layout;
    VkDescriptorSet cbv_desc_set;
    {
        auto lg = std::lock_guard(mMutex);
        // This is a write access to the pool and must be synced
        res = mPool.acquire();
        // This is a write access to mAllocator descriptors
        cbv_desc_set_layout = mAllocatorDescriptors.createSingleCBVLayout();
        cbv_desc_set = mAllocatorDescriptors.allocDescriptor(cbv_desc_set_layout);
    }

    // Perform the initial update to the CBV descriptor set

    // TODO: UNIFORM_BUFFER(_DYNAMIC) cannot be larger than some
    // platform-specific limit, this right here is just a hack
    // We require separate paths in the resource pool (and therefore in the entire API)
    // for "CBV" buffers, and other buffers.
    if (buffer_width < 65536)
    {
        VkDescriptorBufferInfo cbv_info = {};
        cbv_info.buffer = buffer;
        cbv_info.offset = 0;
        cbv_info.range = cc::min(256u, buffer_width);

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = cbv_desc_set;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        write.descriptorCount = 1; // Just one CBV
        write.pBufferInfo = &cbv_info;
        write.dstArrayElement = 0;
        write.dstBinding = spv::cbv_binding_start;

        vkUpdateDescriptorSets(mAllocatorDescriptors.getDevice(), 1, &write, 0, nullptr);
    }

    vkDestroyDescriptorSetLayout(mAllocatorDescriptors.getDevice(), cbv_desc_set_layout, nullptr);

    resource_node& new_node = mPool.get(res);
    new_node.allocation = alloc;
    new_node.type = resource_node::resource_type::buffer;
    new_node.buffer.raw_buffer = buffer;
    new_node.buffer.raw_uniform_dynamic_ds = cbv_desc_set;
    new_node.buffer.width = buffer_width;
    new_node.buffer.stride = buffer_stride;
    new_node.buffer.map = buffer_map;

    new_node.master_state = initial_state;
    new_node.master_state_dependency = util::to_pipeline_stage_dependency(initial_state, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    return {static_cast<handle::index_t>(res)};
}
pr::backend::handle::resource pr::backend::vk::ResourcePool::acquireImage(
    VmaAllocation alloc, VkImage image, format pixel_format, pr::backend::resource_state initial_state, unsigned num_mips, unsigned num_array_layers)
{
    unsigned res;
    {
        // This is a write access to the pool and must be synced
        auto lg = std::lock_guard(mMutex);
        res = mPool.acquire();
    }
    resource_node& new_node = mPool.get(res);
    new_node.allocation = alloc;
    new_node.type = resource_node::resource_type::image;
    new_node.image.raw_image = image;
    new_node.image.pixel_format = pixel_format;
    new_node.image.num_mips = num_mips;
    new_node.image.num_array_layers = num_array_layers;

    new_node.master_state = initial_state;
    new_node.master_state_dependency = util::to_pipeline_stage_dependency(initial_state, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT);

    return {static_cast<handle::index_t>(res)};
}

void pr::backend::vk::ResourcePool::internalFree(resource_node& node)
{
    // This requires no synchronization, as VMA internally syncs
    if (node.type == resource_node::resource_type::image)
    {
        vmaDestroyImage(mAllocator.getAllocator(), node.image.raw_image, node.allocation);
    }
    else
    {
        vmaDestroyBuffer(mAllocator.getAllocator(), node.buffer.raw_buffer, node.allocation);
        // This does require synchronization
        mAllocatorDescriptors.free(node.buffer.raw_uniform_dynamic_ds);
    }
}
