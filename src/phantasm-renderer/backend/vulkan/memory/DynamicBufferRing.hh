#pragma once

#include <cstdint>
#include <cstring>

#include <clean-core/span.hh>

#include <phantasm-renderer/backend/detail/Ring.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

#include "ResourceAllocator.hh"

namespace pr::backend::vk
{
class Device;

// This class mimics the behaviour or the DX11 dynamic buffers. I can hold uniforms, index and vertex buffers.
// It does so by suballocating memory from a huge buffer. The buffer is used in a ring fashion.
// Allocated memory is taken from the tail, freed memory makes the head advance;
// See 'ring.h' to get more details on the ring buffer.
//
// The class knows when to free memory by just knowing:
//    1) the amount of memory used per frame
//    2) the number of backbuffers
//    3) When a new frame just started ( indicated by OnBeginFrame() )
//         - This will free the data of the oldest frame so it can be reused for the new frame
//
// Note than in this ring an allocated chuck of memory has to be contiguous in memory, that is it cannot spawn accross the tail and the head.
// This class takes care of that.

class DynamicBufferRing
{
public:
    void initialize(Device* device, ResourceAllocator* allocator, uint32_t num_backbuffers, uint32_t total_size_in_bytes);
    ~DynamicBufferRing();

    void onBeginFrame();

    bool allocConstantBuffer(uint32_t size, void*& map_ptr, VkDescriptorBufferInfo& out_info);
    bool allocVertexIndexBuffer(uint32_t num_elements, uint32_t element_size_bytes, void*& map_ptr, VkDescriptorBufferInfo& out_info);

    template <class DataT>
    bool allocConstantBufferFromData(DataT const& data, VkDescriptorBufferInfo& out_info)
    {
        void* dest_cpu;
        if (allocConstantBuffer(sizeof(DataT), dest_cpu, out_info))
        {
            std::memcpy(dest_cpu, &data, sizeof(DataT));
            return true;
        }
        return false;
    }

    template <class ElementT>
    bool allocVertexIndexBufferFromData(cc::span<ElementT const> elements, VkDescriptorBufferInfo& out_info)
    {
        void* dest_cpu;
        if (allocVertexIndexBuffer(elements.size(), sizeof(ElementT), dest_cpu, out_info))
        {
            std::memcpy(dest_cpu, elements.data(), sizeof(ElementT) * elements.size());
            return true;
        }
        return false;
    }

    template <class T>
    bool allocConstantBufferTyped(T*& out_data, VkDescriptorBufferInfo& out_info)
    {
        static_assert(!std::is_same_v<T, void>, "Only use this for explicit types");
        static_assert(!std::is_pointer_v<T> && !std::is_reference_v<T>, "T is not the underlying type");
        return allocConstantBuffer(sizeof(T), reinterpret_cast<void*&>(out_data), out_info);
    }

    void setDescriptorSet(uint32_t index, uint32_t size, VkDescriptorSet descriptorSet);

    VkBuffer getBuffer() const { return mBuffer.buffer; }

private:
    Device* mDevice = nullptr;
    ResourceAllocator* mAllocator = nullptr;

    buffer mBuffer;

    char* mData = nullptr;
    backend::detail::RingWithTabs mMemoryRing;
    uint32_t mTotalSize;
};
}
