#include "resource_creation.hh"

pr::backend::vk::buffer pr::backend::vk::create_buffer_from_data(
    pr::backend::vk::Allocator& allocator, pr::backend::vk::UploadHeap& upload_heap, VkBufferUsageFlags usage, size_t size, const void* data)
{
    auto const result = allocator.allocBuffer(uint32_t(size), usage);

    auto* const upload_alloc = upload_heap.suballocateAllowRetry(size, 512);
    std::memcpy(upload_alloc, data, size);
    upload_heap.copyAllocationToBuffer(result.buffer, upload_alloc, size);

    return result;
}
