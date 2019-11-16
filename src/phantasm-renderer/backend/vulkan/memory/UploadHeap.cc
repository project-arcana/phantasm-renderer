#include "UploadHeap.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/detail/byte_util.hh>
#include <phantasm-renderer/backend/vulkan/Device.hh>
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/common/zero_struct.hh>
#include <phantasm-renderer/backend/vulkan/gpu_choice_util.hh>

void pr::backend::vk::UploadHeap::initialize(pr::backend::vk::Device* device, size_t size)
{
    CC_RUNTIME_ASSERT(mDevice == nullptr);
    mDevice = device;

    VkResult res;

    // Create command list and allocators
    {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.queueFamilyIndex = mDevice->getQueueFamilyGraphics();
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        res = vkCreateCommandPool(mDevice->getDevice(), &cmd_pool_info, nullptr, &mCommandPool);
        CC_ASSERT(res == VK_SUCCESS);

        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = mCommandPool;
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = 1;
        res = vkAllocateCommandBuffers(mDevice->getDevice(), &cmd, &mCommandBuffer);
        CC_ASSERT(res == VK_SUCCESS);
    }

    // Create buffer to suballocate
    {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        res = vkCreateBuffer(mDevice->getDevice(), &buffer_info, nullptr, &mUploadHeap);
        CC_ASSERT(res == VK_SUCCESS);

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(mDevice->getDevice(), mUploadHeap, &mem_reqs);

        VkMemoryAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = 0;

        {
            auto const success = memory_type_from_properties(mDevice->getMemoryProperties(), mem_reqs.memoryTypeBits,
                                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, alloc_info.memoryTypeIndex);
            CC_ASSERT(success && "No mappable, coherent memory");
        }

        res = vkAllocateMemory(mDevice->getDevice(), &alloc_info, nullptr, &mUploadHeapMemory);
        CC_ASSERT(res == VK_SUCCESS);

        res = vkBindBufferMemory(mDevice->getDevice(), mUploadHeap, mUploadHeapMemory, 0);
        CC_ASSERT(res == VK_SUCCESS);

        res = vkMapMemory(mDevice->getDevice(), mUploadHeapMemory, 0, mem_reqs.size, 0, (void**)&mDataBegin);
        CC_ASSERT(res == VK_SUCCESS);

        mDataCurrent = mDataBegin;
        mDataEnd = mDataBegin + mem_reqs.size;
    }

    // Create fence
    {
        VkFenceCreateInfo fence_ci;
        fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_ci.pNext = nullptr;
        fence_ci.flags = 0;

        res = vkCreateFence(mDevice->getDevice(), &fence_ci, nullptr, &mFence);
        CC_ASSERT(res == VK_SUCCESS);
    }

    // Begin Command Buffer
    {
        VkCommandBufferBeginInfo cmd_buf_info;
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        cmd_buf_info.pInheritanceInfo = nullptr;

        res = vkBeginCommandBuffer(mCommandBuffer, &cmd_buf_info);
        CC_ASSERT(res == VK_SUCCESS);
    }
}

pr::backend::vk::UploadHeap::~UploadHeap()
{
    if (mDevice)
    {
        auto const device = mDevice->getDevice();

        vkUnmapMemory(device, mUploadHeapMemory);
        vkFreeMemory(device, mUploadHeapMemory, nullptr);
        vkDestroyBuffer(device, mUploadHeap, nullptr);

        vkFreeCommandBuffers(device, mCommandPool, 1, &mCommandBuffer);
        vkDestroyCommandPool(device, mCommandPool, nullptr);

        vkDestroyFence(device, mFence, nullptr);
    }
}

uint8_t* pr::backend::vk::UploadHeap::suballocate(size_t size, size_t align)
{
    mDataCurrent = reinterpret_cast<uint8_t*>(mem::align_offset(reinterpret_cast<size_t>(mDataCurrent), size_t(align)));

    // return NULL if we ran out of space in the heap
    if (mDataCurrent >= mDataEnd || mDataCurrent + size >= mDataEnd)
    {
        return nullptr;
    }

    auto const res = mDataCurrent;
    mDataCurrent += size;
    return res;
}

uint8_t* pr::backend::vk::UploadHeap::suballocateAllowRetry(size_t size, size_t alignment)
{
    auto res = suballocate(size, alignment);
    if (res == nullptr)
    {
        flushAndFinish();
        res = suballocate(size, alignment);
        CC_ASSERT(res != nullptr);
    }
    return res;
}

void pr::backend::vk::UploadHeap::copyAllocationToBuffer(VkBuffer dest_buffer, uint8_t* src_allocation, size_t size)
{
    auto const offset = src_allocation - mDataBegin;
    CC_ASSERT(offset >= 0 && offset <= (mDataEnd - mDataBegin) && "allocation not in range");
    CC_ASSERT(src_allocation + size <= mDataEnd && "allocation copy out of bounds");

    VkBufferCopy region = {};
    region.size = size;
    region.srcOffset = uint32_t(offset);
    region.dstOffset = 0;
    vkCmdCopyBuffer(mCommandBuffer, mUploadHeap, dest_buffer, 1, &region);
}

void pr::backend::vk::UploadHeap::copyAllocationToImage(VkImage dest_image, uint8_t* src_allocation, uint32_t width, uint32_t height, uint32_t mip, uint32_t layer)
{
    auto const offset = src_allocation - mDataBegin;
    CC_ASSERT(offset >= 0 && offset <= (mDataEnd - mDataBegin) && "allocation not in range");

    VkBufferImageCopy region = {};
    region.bufferOffset = uint32_t(offset);
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.mipLevel = mip;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;
    vkCmdCopyBufferToImage(mCommandBuffer, mUploadHeap, dest_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void pr::backend::vk::UploadHeap::flush()
{
    VkMappedMemoryRange range[1] = {};
    range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range[0].memory = mUploadHeapMemory;
    // NOTE: This is not entirely correct, the real size is VkDeviceSize(mDataCurrent - mDataBegin);
    // however, size must either be VK_WHOLE_SIZE or a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize (0x40 on 2080)
    // TODO
    range[0].size = VK_WHOLE_SIZE;
    auto const res = vkFlushMappedMemoryRanges(mDevice->getDevice(), 1, range);
    CC_ASSERT(res == VK_SUCCESS);
}

void pr::backend::vk::UploadHeap::flushAndFinish()
{
    flush();

    // Close
    auto res = vkEndCommandBuffer(mCommandBuffer);
    CC_ASSERT(res == VK_SUCCESS);

    // Submit

    const VkCommandBuffer cmd_bufs[] = {mCommandBuffer};
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_bufs;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;

    res = vkQueueSubmit(mDevice->getQueueGraphics(), 1, &submit_info, mFence);
    CC_ASSERT(res == VK_SUCCESS);

    // Make sure it's been processed by the GPU

    res = vkWaitForFences(mDevice->getDevice(), 1, &mFence, VK_TRUE, UINT64_MAX);
    CC_ASSERT(res == VK_SUCCESS);

    vkResetFences(mDevice->getDevice(), 1, &mFence);

    // Reset so it can be reused
    VkCommandBufferBeginInfo cmd_buf_info;
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = nullptr;
    cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    cmd_buf_info.pInheritanceInfo = nullptr;

    res = vkBeginCommandBuffer(mCommandBuffer, &cmd_buf_info);
    CC_ASSERT(res == VK_SUCCESS);

    mDataCurrent = mDataBegin;
}
