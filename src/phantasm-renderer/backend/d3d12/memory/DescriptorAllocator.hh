#pragma once

#include <memory> // TODO: cc::shared_ptr
#include <mutex>
#include <set>
#include <vector>

#include <d3d12.h>

#include <clean-core/typedefs.hh>

#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
//#include <phantasm-renderer/backend/d3d12/common/sanitized_d3d12.hh>

#include "byte_util.hh"

namespace pr::backend::d3d12
{
/// Freelist allocator for CPU-visible descriptors
/// Adapted from http://diligentgraphics.com/diligent-engine/architecture/d3d12/variable-size-memory-allocations-manager/
/// Internally synchronized
class DescriptorAllocator
{
public:
    struct allocation
    {
        void* cpu_ptr;
        D3D12_GPU_VIRTUAL_ADDRESS gpu_ptr;
    };


public:
    /// Page size should fit all allocations of a single command list
    explicit DescriptorAllocator(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned heap_size = 256u);

    DescriptorAllocator(DescriptorAllocator const&) = delete;
    DescriptorAllocator(DescriptorAllocator&&) noexcept = delete;
    DescriptorAllocator& operator=(DescriptorAllocator const&) = delete;
    DescriptorAllocator& operator=(DescriptorAllocator&&) noexcept = delete;

    /// Allocates the given amount of descriptors
    [[nodiscard]] allocation allocate(unsigned num_descriptors = 1);

    /// Frees descriptors that are no longer in flight
    void cleanUp(cc::uint64 frame_generation);

private:
    struct heap
    {
    };
    using shared_heap_t = std::shared_ptr<heap>;
    using pool_t = std::vector<shared_heap_t>;

private:
    [[nodiscard]] shared_heap_t createHeap();

private:
    /// The type of all contained heaps
    D3D12_DESCRIPTOR_HEAP_TYPE const mType;

    /// The amount of descriptors in each heap
    unsigned mHeapSize;

    pool_t mPool;

    /// Indices of free heaps in mPool
    std::set<size_t> mFreeHeapIndices;

    // TODO: Benchmark against td spinlock
    std::mutex mMutex;
};

}
