#pragma once

#include <type_traits>

#include <clean-core/span.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
#include <phantasm-renderer/backend/detail/Ring.hh>

namespace pr::backend::d3d12
{
// This class mimics the behaviour or the DX11 dynamic buffers.
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
// Note than in this ring an allocated chunk of memory has to be contiguous in memory, that is it cannot spawn accross the tail and the head.
// This class takes care of that.

class DynamicBufferRing
{
public:
    void initialize(ID3D12Device& device, uint32_t num_backbuffers, uint32_t total_size_in_bytes);

    void onBeginFrame();

    bool allocIndexBuffer(uint32_t num_indices, uint32_t stride_in_bytes, void*& out_data, D3D12_INDEX_BUFFER_VIEW& out_view);

    bool allocVertexBuffer(uint32_t num_vertices, uint32_t stride_in_bytes, void*& out_data, D3D12_VERTEX_BUFFER_VIEW& out_view);

    template <class VertT>
    bool allocVertexBufferFromData(cc::span<VertT const> vertices, D3D12_VERTEX_BUFFER_VIEW& out_view)
    {
        void* dest_cpu;
        if (allocVertexBuffer(vertices.size(), sizeof(VertT), dest_cpu, out_view))
        {
            ::memcpy(dest_cpu, vertices.data(), sizeof(VertT) * vertices.size());
            return true;
        }
        return false;
    }

    bool allocConstantBuffer(uint32_t size, void*& out_data, D3D12_GPU_VIRTUAL_ADDRESS& out_view);

    bool allocConstantBufferFromData(void* src_data, uint32_t size, D3D12_GPU_VIRTUAL_ADDRESS& out_view)
    {
        void* dest_cpu;
        if (allocConstantBuffer(size, dest_cpu, out_view))
        {
            ::memcpy(dest_cpu, src_data, size);
            return true;
        }
        return false;
    }

    template <class T>
    bool allocConstantBufferTyped(T*& out_data, D3D12_GPU_VIRTUAL_ADDRESS& out_view)
    {
        static_assert(!std::is_same_v<T, void>, "Only use this for explicit types");
        static_assert(!std::is_pointer_v<T> && !std::is_reference_v<T>, "T is not the underlying type");
        return allocConstantBuffer(sizeof(T), reinterpret_cast<void*&>(out_data), out_view);
    }

private:
    shared_com_ptr<ID3D12Resource> mResource;
    char* mData = nullptr;
    backend::detail::RingWithTabs mMemoryRing;
    uint32_t mTotalSize; ///< The total allocated size of this ring in bytes
};
}
