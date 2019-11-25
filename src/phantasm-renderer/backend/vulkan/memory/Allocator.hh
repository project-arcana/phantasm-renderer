#pragma once

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace pr::backend::assets
{
struct image_size;
};

namespace pr::backend::vk
{
struct buffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct image
{
    VkImage image;
    VmaAllocation allocation;
};

class Allocator
{
public:
    void initialize(VkPhysicalDevice physical, VkDevice device);
    void destroy();

    [[nodiscard]] buffer allocBuffer(uint32_t size, VkBufferUsageFlags usage);

    [[nodiscard]] image allocAssetTexture(assets::image_size const& image_size, bool use_srgb = false);

    [[nodiscard]] image allocRenderTarget(unsigned width, unsigned height, VkFormat format, VkImageUsageFlags usage);

    [[nodiscard]] buffer allocCPUtoGPUBuffer(uint32_t size, VkBufferUsageFlags usage, void** map_ptr);

    void unmapCPUtoGPUBuffer(buffer const& buffer);

    void free(buffer const& buffer);
    void free(image const& image);

private:
    VmaAllocator mAllocator = nullptr;
};

}
