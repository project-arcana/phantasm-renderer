#pragma once

#include <map>
#include <memory> // TODO: cc::shared_ptr
#include <mutex>
#include <queue>
#include <set>
#include <vector>

#include <clean-core/assert.hh>
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
private:
    struct heap;

public:
    struct allocation
    {
        /// null-allocation ctor
        explicit allocation() = default;
        explicit allocation(D3D12_CPU_DESCRIPTOR_HANDLE handle, int num_handles, int descriptor_size, heap* parent);

    public:
        /// Returns true iff this allocation is valid
        [[nodiscard]] bool is_valid() const { return _handle.ptr != 0; }

        /// Returns the handle at the given index
        [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE get_handle(int i = 0) const
        {
            CC_ASSERT(i >= 0 && i < _num_handles);
            return {_handle.ptr + size_t(_handle_size * i)};
        }

        /// Returns the amount of handles
        [[nodiscard]] int get_num_handles() const { return _num_handles; }

        /// Explicit free
        void free(cc::uint64 current_frame_gen);

        heap* _parent_heap = nullptr;
    private:
        D3D12_CPU_DESCRIPTOR_HANDLE _handle = {0};
        int _num_handles = -1; ///< Amount of contiguous handles in this allocation
        int _handle_size = -1; ///< Size of each individual handle in bytes
    };


public:
    /// Page size should fit all allocations of a single command list
    explicit DescriptorAllocator(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, int heap_size = 256);

    DescriptorAllocator(DescriptorAllocator const&) = delete;
    DescriptorAllocator(DescriptorAllocator&&) noexcept = delete;
    DescriptorAllocator& operator=(DescriptorAllocator const&) = delete;
    DescriptorAllocator& operator=(DescriptorAllocator&&) noexcept = delete;

    /// Allocates the given amount of descriptors, can be called from any thread, internally synchronized
    [[nodiscard]] allocation allocate(int num_descriptors = 1);

    /// Frees descriptors from or from before the given generation, can be called from any thread, internally synchronized
    void cleanUp(cc::uint64 frame_generation);

private:
    struct heap
    {
        explicit heap(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, int size);

        [[nodiscard]] allocation allocate(int num_descriptors);

        void free(allocation&& alloc, cc::uint64 frame_generation);

        void clean_up(cc::uint64 frame_generation);

        [[nodiscard]] D3D12_DESCRIPTOR_HEAP_TYPE get_type() const { return _type; }

        [[nodiscard]] bool can_fit(int num_descriptors) const;

        [[nodiscard]] int get_num_free_handles() const { return _num_free_handles; }

    private:
        /// Computes the offset of the descriptor handle from the start of the heap
        int compute_offset(D3D12_CPU_DESCRIPTOR_HANDLE const& handle) const;

        /// Adds a new block to the free list
        void add_block(int offset, int num_descriptors);

        /// Frees a block of descriptors
        void free_block(int offset, int num_descriptors);

    private:
        using heap_offset_t = int;
        using heap_size_t = int;

        struct free_block_info;

        // TODO: unordered_, TODO: not STL, TODO: entirely different representation
        using free_blocks_by_offset = std::map<heap_offset_t, free_block_info>;
        using free_blocks_by_size = std::multimap<heap_size_t, free_blocks_by_offset::iterator>;

        struct free_block_info
        {
            free_block_info(heap_size_t s) : size(s) {}
            heap_size_t size;
            free_blocks_by_size::iterator iterator;
        };

        struct stale_descriptor_info
        {
            stale_descriptor_info(heap_offset_t off, heap_size_t s, cc::uint64 gen) : offset(off), size(s), frame_generation(gen) {}
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

        cc::uint32 _increment_size;
        int _num_descriptors;
        int _num_free_handles;

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
    int mHeapSize;

    pool_t mPool;

    /// Indices of free heaps in mPool
    std::set<size_t> mFreeHeapIndices;

    // TODO: Benchmark against td spinlock
    std::mutex mMutex;
};

}
