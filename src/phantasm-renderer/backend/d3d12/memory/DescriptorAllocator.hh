#pragma once

#include <map>
#include <memory> // TODO: cc::shared_ptr
#include <mutex>
#include <queue>
#include <set>
#include <vector>

#include <clean-core/typedefs.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>

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

        [[nodiscard]] bool is_valid();
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
        explicit heap(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned size);

        [[nodiscard]] allocation allocate(unsigned num_descriptors);

        void free(allocation&& alloc, cc::uint64 frame_generation);

        void clean_up(cc::uint64 frame_generation);

        [[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE get_type() const { return _type; }

        [[nodiscard]] bool can_fit(unsigned num_descriptors) const;

        [[nodiscard]] unsigned get_num_free_handles() const;

    private:
        /// Computes the offset of the descriptor handle from the start of the heap
        unsigned compute_offset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

        /// Adds a new block to the free list
        void add_block(unsigned offset, unsigned num_descriptors);

        /// Frees a block of descriptors
        void free_block(unsigned offset, unsigned num_descriptors);

    private:
        using heap_offset_t = unsigned;
        using heap_size_t = unsigned;

        struct free_block_info;

        // TODO: unordered_, TODO: not STL, TODO: entirely different representation
        using free_blocks_by_offset = std::map<heap_offset_t, free_block_info>;
        using free_blocks_by_size = std::multimap<heap_size_t, free_blocks_by_offset::iterator>;

        struct free_block_info
        {
            heap_size_t size;
            free_blocks_by_size::iterator iterator;
        };

        struct stale_descriptor_info
        {
            heap_offset_t offset;
            heap_size_t size;
            cc::uint64 frame_generation;
        };

        using stale_descriptor_queue = std::queue<stale_descriptor_info>;

    private:
        free_blocks_by_offset _free_blocks_by_offset;
        free_blocks_by_size _free_blocks_by_size;
        stale_descriptor_queue _stale_descriptors;

        shared_com_ptr<ID3D12DescriptorHeap> _descriptor_heap;
        D3D12_DESCRIPTOR_HEAP_TYPE const _type;
        CD3DX12_CPU_DESCRIPTOR_HANDLE _base_descriptor;

        unsigned _increment_size;
        unsigned _num_descriptors;
        unsigned _num_free_handles;

        std::mutex _mutex;
    };
    using shared_heap_t = std::shared_ptr<heap>;
    using pool_t = std::vector<shared_heap_t>;

private:
    [[nodiscard]] shared_heap_t createHeap();

private:
    /// The parent device
    ID3D12Device& mDevice;

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
