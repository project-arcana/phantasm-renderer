#include "CommandList.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/capped_vector.hh>

#include "BackendD3D12.hh"

void pr::backend::d3d12::CommandList::initialize(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator& allocator)
{
    mType = type;
    PR_D3D12_VERIFY(device.CreateCommandList(0, mType, &allocator, nullptr, PR_COM_WRITE(mCommandList)));
}

void pr::backend::d3d12::CommandList::setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap)
{
    if (mDescriptorHeaps[type] != heap)
    {
        mDescriptorHeaps[type] = heap;
        rebindDescriptorHeaps();
    }
}

void pr::backend::d3d12::CommandList::close() { mCommandList->Close(); }

void pr::backend::d3d12::CommandList::rebindDescriptorHeaps()
{
    cc::capped_vector<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> packed_desc_heaps;

    for (auto const& heap : mDescriptorHeaps)
        if (heap)
            packed_desc_heaps.push_back(heap);

    mCommandList->SetDescriptorHeaps(UINT(packed_desc_heaps.size()), packed_desc_heaps.data());
}

void pr::backend::d3d12::CommandListRing::initialize(pr::backend::d3d12::BackendD3D12& backend,
                                                     unsigned num_allocators,
                                                     unsigned num_command_lists_per_allocator,
                                                     const D3D12_COMMAND_QUEUE_DESC& queue_desc)
{
    mRing = cc::array<ring_entry>::defaulted(num_allocators);

    auto& device = backend.mDevice.getDevice();
    for (auto& ring_entry : mRing)
    {
        // Create the allocator
        PR_D3D12_VERIFY(device.CreateCommandAllocator(queue_desc.Type, PR_COM_WRITE(ring_entry.command_allocator)));

        // Create the command lists
        ring_entry.command_lists = decltype(ring_entry.command_lists)::defaulted(num_command_lists_per_allocator);
        for (auto& list : ring_entry.command_lists)
        {
            PR_D3D12_VERIFY(device.CreateCommandList(0, queue_desc.Type, ring_entry.command_allocator, nullptr, PR_COM_WRITE(list)));
            list->Close();
        }

        ring_entry.num_command_lists_in_flight = 0;
    }

    // Execute all (closed) lists once to suppress warnings
    auto& direct_queue = backend.mDirectQueue.getQueue();
    for (auto& ring_entry : mRing)
    {
        direct_queue.ExecuteCommandLists(                                                //
            UINT(ring_entry.command_lists.size()),                                       //
            reinterpret_cast<ID3D12CommandList* const*>(ring_entry.command_lists.data()) // This is hopefully always fine
        );
    }

    backend.flushGPU();

    mRingIndex = 0;
    mActiveEntry = &mRing[unsigned(mRingIndex)];
}

void pr::backend::d3d12::CommandListRing::onBeginFrame()
{
    ++mRingIndex;
    // faster than modulo
    if (mRingIndex >= int(mRing.size()))
        mRingIndex -= int(mRing.size());

    mActiveEntry = &mRing[unsigned(mRingIndex)];

    // reset the entry
    PR_D3D12_VERIFY(mActiveEntry->command_allocator->Reset());
    mActiveEntry->num_command_lists_in_flight = 0;
}

ID3D12GraphicsCommandList* pr::backend::d3d12::CommandListRing::acquireCommandList()
{
    return mActiveEntry->acquire_list();
}

ID3D12GraphicsCommandList* pr::backend::d3d12::CommandListRing::ring_entry::acquire_list()
{
    // No need to assert, cc::array internally asserts
    auto* const res = command_lists[unsigned(num_command_lists_in_flight++)].get();
    PR_D3D12_VERIFY(res->Reset(command_allocator, nullptr));
    return res;
}

#endif
