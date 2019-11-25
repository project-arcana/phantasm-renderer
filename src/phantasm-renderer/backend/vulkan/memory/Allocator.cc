#include "Allocator.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/assets/image_loader.hh>
#include <phantasm-renderer/backend/vulkan/common/verify.hh>

#include "VMA.hh"

void pr::backend::vk::Allocator::initialize(VkPhysicalDevice physical, VkDevice device)
{
    CC_ASSERT(mAllocator == nullptr);

    VmaAllocatorCreateInfo create_info = {};
    create_info.physicalDevice = physical;
    create_info.device = device;

    PR_VK_VERIFY_SUCCESS(vmaCreateAllocator(&create_info, &mAllocator));
}

void pr::backend::vk::Allocator::destroy() { vmaDestroyAllocator(mAllocator); }

pr::backend::vk::buffer pr::backend::vk::Allocator::allocBuffer(uint32_t size, VkBufferUsageFlags usage)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    buffer res;
    PR_VK_VERIFY_SUCCESS(vmaCreateBuffer(mAllocator, &buffer_info, &alloc_info, &res.buffer, &res.allocation, nullptr));
    return res;
}

pr::backend::vk::image pr::backend::vk::Allocator::allocAssetTexture(const pr::backend::assets::image_size& image_size, bool use_srgb)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;

    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = use_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

    image_info.extent.width = uint32_t(image_size.width);
    image_info.extent.height = uint32_t(image_size.height);
    image_info.extent.depth = 1;
    image_info.mipLevels = uint32_t(image_size.num_mipmaps);
    image_info.arrayLayers = uint32_t(image_size.array_size);

    image_info.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: Configurable
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = nullptr;

    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_info.flags = 0;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

    if (image_size.array_size == 6)
        image_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_info.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;

    image res;
    PR_VK_VERIFY_SUCCESS(vmaCreateImage(mAllocator, &image_info, &alloc_info, &res.image, &res.allocation, nullptr));
    return res;
}

pr::backend::vk::image pr::backend::vk::Allocator::allocRenderTarget(unsigned width, unsigned height, VkFormat format, VkImageUsageFlags usage)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = nullptr;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: Configurable
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.queueFamilyIndexCount = 0;
    image_info.pQueueFamilyIndices = nullptr;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = usage;
    image_info.flags = 0;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_info.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;

    image res;
    PR_VK_VERIFY_SUCCESS(vmaCreateImage(mAllocator, &image_info, &alloc_info, &res.image, &res.allocation, nullptr));
    return res;
}

pr::backend::vk::buffer pr::backend::vk::Allocator::allocCPUtoGPUBuffer(uint32_t size, VkBufferUsageFlags usage, void** map_ptr)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;

    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    alloc_info.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;

    buffer res;
    PR_VK_VERIFY_SUCCESS(vmaCreateBuffer(mAllocator, &buffer_info, &alloc_info, &res.buffer, &res.allocation, nullptr));
    PR_VK_VERIFY_SUCCESS(vmaMapMemory(mAllocator, res.allocation, map_ptr));
    return res;
}

void pr::backend::vk::Allocator::unmapCPUtoGPUBuffer(const pr::backend::vk::buffer& buffer) { vmaUnmapMemory(mAllocator, buffer.allocation); }

void pr::backend::vk::Allocator::free(const pr::backend::vk::buffer& buffer) { vmaDestroyBuffer(mAllocator, buffer.buffer, buffer.allocation); }

void pr::backend::vk::Allocator::free(const pr::backend::vk::image& image) { vmaDestroyImage(mAllocator, image.image, image.allocation); }
