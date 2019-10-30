#include "DescriptorAllocator.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

pr::backend::d3d12::DescriptorAllocator::heap::heap(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, int size)
  : _type(type), _num_descriptors(size), _num_free_handles(_num_descriptors)
{
    CC_ASSERT(_num_descriptors > 0);

    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heap_desc.NodeMask = 0;
    heap_desc.Type = _type;
    heap_desc.NumDescriptors = UINT(_num_descriptors);

    PR_D3D12_VERIFY(device.CreateDescriptorHeap(&heap_desc, PR_COM_WRITE(_descriptor_heap)));

    _base_descriptor = _descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    _increment_size = device.GetDescriptorHandleIncrementSize(_type);

    add_block(0, _num_free_handles);
}

pr::backend::d3d12::DescriptorAllocator::allocation pr::backend::d3d12::DescriptorAllocator::heap::allocate(int num)
{
    CC_ASSERT(num > 0);

    std::lock_guard lg(_mutex);

    if (num > _num_free_handles)
        return allocation{}; // heap full, return a null-allocation

    // Get the smallest block that can fit the allocation
    auto const it_smallest_block = _free_blocks_by_size.lower_bound(num);

    if (it_smallest_block == _free_blocks_by_size.end())
        return allocation{}; // no blocks can satisfy the allocation, return a null-allocation

    auto const it_offset = it_smallest_block->second;

    auto const block_size = it_smallest_block->first;
    auto const block_offset = it_offset->first;

    // Remove the free block from the lists
    _free_blocks_by_size.erase(it_smallest_block);
    _free_blocks_by_offset.erase(it_offset);

    if (block_size > num)
    {
        // the free block is not entirely consumed, return the rest to the free list
        auto const new_offset = block_offset + num;
        auto const new_size = block_size - num;
        add_block(new_offset, new_size);
    }

    _num_free_handles -= num;
    return allocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(_base_descriptor, block_offset, _increment_size), num, int(_increment_size), this);
}

void pr::backend::d3d12::DescriptorAllocator::heap::free(pr::backend::d3d12::DescriptorAllocator::allocation&& alloc, cc::uint64 frame_generation)
{
    auto const offset = compute_offset(alloc.get_handle());

    std::lock_guard lg(_mutex);
    _stale_descriptors.emplace(offset, alloc.get_num_handles(), frame_generation);
}

void pr::backend::d3d12::DescriptorAllocator::heap::clean_up(cc::uint64 frame_generation)
{
    std::lock_guard lg(_mutex);

    // Free all stale descriptors from the front that are from the freed generation, or older
    while (!_stale_descriptors.empty() && _stale_descriptors.front().frame_generation <= frame_generation)
    {
        auto const& stale_descriptor = _stale_descriptors.front();
        free_block(stale_descriptor.offset, stale_descriptor.size);
        _stale_descriptors.pop();
    }
}

bool pr::backend::d3d12::DescriptorAllocator::heap::can_fit(int num) const
{
    CC_ASSERT(num > 0);
    // The allocation fits iff a free list with a size >= num_descriptors exists
    return _free_blocks_by_size.lower_bound(num) != _free_blocks_by_size.end();
}

int pr::backend::d3d12::DescriptorAllocator::heap::compute_offset(D3D12_CPU_DESCRIPTOR_HANDLE const& handle) const
{
    return int(handle.ptr - _base_descriptor.ptr) / int(_increment_size);
}

void pr::backend::d3d12::DescriptorAllocator::heap::add_block(int offset, int size)
{
    // create the new block
    auto const it_offset = _free_blocks_by_offset.emplace(offset, size);

    // add its iterator into the size map
    auto const it_size = _free_blocks_by_size.emplace(size, it_offset.first);

    // amend the size map iterator to the created block
    it_offset.first->second.iterator = it_size;
}

void pr::backend::d3d12::DescriptorAllocator::heap::free_block(int offset, int size)
{
    // Find the first element whose offset is greater than the specified offset.
    // This is the block that should appear after the block that is being freed.
    auto const next_block_it = _free_blocks_by_offset.upper_bound(offset);

    // Find the block that appears before the block being freed.
    auto it_prev_block = next_block_it;
    // If it's not the first block in the list.
    if (it_prev_block != _free_blocks_by_offset.begin())
    {
        // Go to the previous block in the list.
        --it_prev_block;
    }
    else
    {
        // Otherwise, just set it to the end of the list to indicate that no
        // block comes before the one being freed.
        it_prev_block = _free_blocks_by_offset.end();
    }

    // Add the number of free handles back to the heap.
    // This needs to be done before merging any blocks since merging
    // blocks modifies the numDescriptors variable.
    _num_free_handles += size;

    if (it_prev_block != _free_blocks_by_offset.end() && offset == it_prev_block->first + it_prev_block->second.size)
    {
        // The previous block is exactly behind the block that is to be freed.
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //

        // Increase the block size by the size of merging with the previous block.
        offset = it_prev_block->first;
        size += it_prev_block->second.size;

        // Remove the previous block from the free list.
        _free_blocks_by_size.erase(it_prev_block->second.iterator);
        _free_blocks_by_offset.erase(it_prev_block);
    }

    if (next_block_it != _free_blocks_by_offset.end() && offset + size == next_block_it->first)
    {
        // The next block is exactly in front of the block that is to be freed.
        //
        // Offset               NextBlock.Offset
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|

        // Increase the block size by the size of merging with the next block.
        size += next_block_it->second.size;

        // Remove the next block from the free list.
        _free_blocks_by_size.erase(next_block_it->second.iterator);
        _free_blocks_by_offset.erase(next_block_it);
    }

    // Add the freed block to the free list.
    add_block(offset, size);
}

#endif
