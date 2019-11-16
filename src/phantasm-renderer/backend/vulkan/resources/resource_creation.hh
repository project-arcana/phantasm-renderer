#pragma once

#include <phantasm-renderer/backend/vulkan/memory/Allocator.hh>
#include <phantasm-renderer/backend/vulkan/memory/UploadHeap.hh>

namespace pr::backend::vk
{
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
