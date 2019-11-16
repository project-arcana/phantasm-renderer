#pragma once

#include <phantasm-renderer/backend/vulkan/memory/Allocator.hh>
#include <phantasm-renderer/backend/vulkan/memory/UploadHeap.hh>

namespace pr::backend::vk
{
//
// create resources
//

[[nodiscard]] inline image create_render_target(Allocator& allocator, unsigned w, unsigned h, VkFormat format)
{
    return allocator.allocRenderTarget(w, h, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
}

[[nodiscard]] inline image create_depth_stencil(Allocator& allocator, unsigned w, unsigned h, VkFormat format)
{
    return allocator.allocRenderTarget(w, h, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
}

//
// create views on resources
//

[[nodiscard]] VkImageView make_image_view(VkDevice device, image const& image, VkFormat format, unsigned num_mips = 1, int mip = -1);

//
// create initialized resources from files or data
// variants using an upload buffer only initiate copies, and do not flush
//

/// resulting access mask: VK_ACCESS_SHADER_READ_BIT
[[nodiscard]] image create_texture_from_file(Allocator& allocator, UploadHeap& upload_heap, char const* filename, bool use_srgb = false);

[[nodiscard]] buffer create_buffer_from_data(Allocator& allocator, UploadHeap& upload_heap, VkBufferUsageFlags usage, size_t size, void const* data);

template <class ElementT>
[[nodiscard]] buffer create_vertex_buffer_from_data(Allocator& allocator, UploadHeap& upload_heap, cc::span<ElementT const> elements)
{
    return create_buffer_from_data(allocator, upload_heap, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   elements.size() * sizeof(ElementT), elements.data());
}

template <class ElementT>
[[nodiscard]] buffer create_index_buffer_from_data(Allocator& allocator, UploadHeap& upload_heap, cc::span<ElementT const> elements)
{
    return create_buffer_from_data(allocator, upload_heap, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   elements.size() * sizeof(ElementT), elements.data());
}
}
