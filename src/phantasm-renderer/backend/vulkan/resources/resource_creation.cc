#include "resource_creation.hh"

#include <cstdint>

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <typed-geometry/tg.hh>

#include <phantasm-renderer/backend/assets/image_loader.hh>

#include "resource_state.hh"

VkImageView pr::backend::vk::make_image_view(VkDevice device, const pr::backend::vk::image& image, VkFormat format, unsigned num_mips, int mip)
{
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image.image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    if (format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D24_UNORM_S8_UINT) // TODO
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (mip == -1)
    {
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = num_mips;
    }
    else
    {
        info.subresourceRange.baseMipLevel = uint32_t(mip);
        info.subresourceRange.levelCount = 1;
    }

    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    VkImageView res;
    auto const vr = vkCreateImageView(device, &info, nullptr, &res);
    CC_ASSERT(vr == VK_SUCCESS);
    return res;
}

pr::backend::vk::image pr::backend::vk::create_texture_from_file(pr::backend::vk::ResourceAllocator& allocator, pr::backend::vk::UploadHeap& upload_heap, const char* filename, bool use_srgb)
{
    assets::image_size img_size;
    auto const img_data = assets::load_image(filename, img_size);

    CC_RUNTIME_ASSERT(assets::is_valid(img_data) && "Image file invalid");

    image res = allocator.allocAssetTexture(img_size, use_srgb);

    // Upload Image
    {
        {
            auto const barrier = get_image_memory_barrier(res.image, state_change{resource_state::undefined, resource_state::copy_dest}, false,
                                                          img_size.num_mipmaps, img_size.array_size);
            vkCmdPipelineBarrier(upload_heap.getCommandBuffer(), VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }

        auto const bytes_per_pixel = 4u;

        for (auto a = 0u; a < img_size.array_size; a++)
        {
            for (auto mip = 0u; mip < img_size.num_mipmaps; mip++)
            {
                auto const mip_width = cc::max(unsigned(tg::floor(img_size.width / tg::pow(2.f, float(mip)))), 1u);
                auto const mip_height = cc::max(unsigned(tg::floor(img_size.height / tg::pow(2.f, float(mip)))), 1u);
                auto const mip_size_bytes = mip_width * mip_height * bytes_per_pixel;

                auto* const pixels = upload_heap.suballocateAllowRetry(mip_size_bytes, 512);

                auto const offset = uint32_t(pixels - upload_heap.getBasePointer());

                assets::copy_subdata(img_data, pixels, mip_width * bytes_per_pixel, mip_width * bytes_per_pixel, mip_height);

                {
                    VkBufferImageCopy region = {};
                    region.bufferOffset = offset;
                    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.imageSubresource.layerCount = 1;
                    region.imageSubresource.baseArrayLayer = a;
                    region.imageSubresource.mipLevel = mip;
                    region.imageExtent.width = mip_width;
                    region.imageExtent.height = mip_height;
                    region.imageExtent.depth = 1;
                    vkCmdCopyBufferToImage(upload_heap.getCommandBuffer(), upload_heap.getResource(), res.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
                }
            }
        }

        // prepare to shader read
        //
        {
            auto const barrier = get_image_memory_barrier(res.image, state_change{resource_state::copy_dest, resource_state::shader_resource}, false,
                                                          img_size.num_mipmaps, img_size.array_size);
            vkCmdPipelineBarrier(upload_heap.getCommandBuffer(), VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                                 0, nullptr, 1, &barrier);
        }
    }

    assets::free(img_data);
    return res;
}

pr::backend::vk::buffer pr::backend::vk::create_buffer_from_data(
    pr::backend::vk::ResourceAllocator& allocator, pr::backend::vk::UploadHeap& upload_heap, VkBufferUsageFlags usage, size_t size, const void* data)
{
    auto const result = allocator.allocBuffer(uint32_t(size), usage);

    auto* const upload_alloc = upload_heap.suballocateAllowRetry(size, 512);
    std::memcpy(upload_alloc, data, size);
    upload_heap.copyAllocationToBuffer(result.buffer, upload_alloc, size);

    return result;
}
