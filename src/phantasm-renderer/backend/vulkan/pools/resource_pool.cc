#include "resource_pool.hh"

#include <iostream>

#include <clean-core/bit_cast.hh>

#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/common/vk_format.hh>
#include <phantasm-renderer/backend/vulkan/memory/VMA.hh>

pr::backend::handle::resource pr::backend::vk::ResourcePool::createTexture2D(pr::backend::format format, int w, int h, int mips)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;

    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = util::to_vk_format(format);

    image_info.extent.width = uint32_t(w);
    image_info.extent.height = uint32_t(h);
    image_info.extent.depth = 1;
    image_info.mipLevels = uint32_t(mips);
    image_info.arrayLayers = uint32_t(0u);

    image_info.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: Configurable
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = nullptr;

    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.flags = 0;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

    // if (image_size.array_size == 6)
    //    image_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_info.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;

    VmaAllocation res_alloc;
    VkImage res_image;
    PR_VK_VERIFY_SUCCESS(vmaCreateImage(mAllocator.getAllocator(), &image_info, &alloc_info, &res_image, &res_alloc, nullptr));
    return acquireImage(res_alloc, res_image, resource_state::undefined);
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::createRenderTarget(pr::backend::format format, int w, int h)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = util::to_vk_format(format);
    image_info.extent.width = w;
    image_info.extent.height = h;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: Configurable
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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
    alloc_info.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;

    VmaAllocation res_alloc;
    VkImage res_image;
    PR_VK_VERIFY_SUCCESS(vmaCreateImage(mAllocator.getAllocator(), &image_info, &alloc_info, &res_image, &res_alloc, nullptr));
    return acquireImage(res_alloc, res_image, resource_state::undefined);
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::createBuffer(unsigned size_bytes, pr::backend::resource_state, unsigned stride_bytes)
{
    // TODO: we ignore the initial state here entirely

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size_bytes;
    // right now we'll just take all usages this thing might have in API semantics
    // it might be required down the line to restrict this (as in, make it part of API)
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                        | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

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
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                        | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc_info.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;

    VmaAllocation res_alloc;
    VkBuffer res_buffer;
    PR_VK_VERIFY_SUCCESS(vmaCreateBuffer(mAllocator.getAllocator(), &buffer_info, &alloc_info, &res_buffer, &res_alloc, nullptr));
    void* data_start_void;
    PR_VK_VERIFY_SUCCESS(vmaMapMemory(mAllocator.getAllocator(), res_alloc, &data_start_void));
    return acquireBuffer(res_alloc, res_buffer, resource_state::undefined, size_bytes, stride_bytes, cc::bit_cast<std::byte*>(data_start_void));
}

void pr::backend::vk::ResourcePool::free(pr::backend::handle::resource res)
{
    CC_ASSERT(res != mInjectedBackbufferResource && "the backbuffer resource must not be freed");

    resource_node& freed_node = mPool.get(static_cast<unsigned>(res.index));
    internalFree(freed_node);

    {
        // This is a write access to the pool and must be synced
        auto lg = std::lock_guard(mMutex);
        mPool.release(static_cast<unsigned>(res.index));
    }
}

void pr::backend::vk::ResourcePool::initialize(VkPhysicalDevice physical, VkDevice device, unsigned max_num_resources)
{
    mAllocator.initialize(physical, device);
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
}

pr::backend::handle::resource pr::backend::vk::ResourcePool::acquireBuffer(
    VmaAllocation alloc, VkBuffer buffer, pr::backend::resource_state initial_state, unsigned buffer_width, unsigned buffer_stride, std::byte* buffer_map)
{
    unsigned res;
    {
        // This is a write access to the pool and must be synced
        auto lg = std::lock_guard(mMutex);
        res = mPool.acquire();
    }
    resource_node& new_node = mPool.get(res);
    new_node.allocation = alloc;
    new_node.buffer = buffer;

    new_node.master_state = initial_state;

    new_node.buffer_width = buffer_width;
    new_node.buffer_stride = buffer_stride;
    new_node.buffer_map = buffer_map;
    new_node.type = resource_node::resource_type::buffer;

    return {static_cast<handle::index_t>(res)};
}
pr::backend::handle::resource pr::backend::vk::ResourcePool::acquireImage(VmaAllocation alloc, VkImage image, pr::backend::resource_state initial_state)
{
    unsigned res;
    {
        // This is a write access to the pool and must be synced
        auto lg = std::lock_guard(mMutex);
        res = mPool.acquire();
    }
    resource_node& new_node = mPool.get(res);
    new_node.allocation = alloc;
    new_node.image = image;

    new_node.master_state = initial_state;

    new_node.buffer_width = 0;
    new_node.buffer_stride = 0;
    new_node.buffer_map = nullptr;
    new_node.type = resource_node::resource_type::image;

    return {static_cast<handle::index_t>(res)};
}

void pr::backend::vk::ResourcePool::internalFree(resource_node& node)
{
    // This requires no synchronization, as VMA internally syncs
    if (node.type == resource_node::resource_type::image)
    {
        vmaDestroyImage(mAllocator.getAllocator(), node.image, node.allocation);
    }
    else
    {
        vmaDestroyBuffer(mAllocator.getAllocator(), node.buffer, node.allocation);
    }
}
