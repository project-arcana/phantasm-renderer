#include "CommandList.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/capped_vector.hh>

void pr::backend::d3d12::CommandList::initialize(ID3D12Device &device, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator &allocator)
{
    mType = type;
    PR_D3D12_VERIFY(device.CreateCommandList(0, mType, &allocator, nullptr, PR_COM_WRITE(mCommandList)));
}

void pr::backend::d3d12::CommandList::setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap *heap)
{
    if (mDescriptorHeaps[type] != heap)
    {
        mDescriptorHeaps[type] = heap;
        rebindDescriptorHeaps();
    }
}

void pr::backend::d3d12::CommandList::close()
{
    mCommandList->Close();
}

void pr::backend::d3d12::CommandList::rebindDescriptorHeaps()
{
    cc::capped_vector<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> packed_desc_heaps;

    for (auto const& heap : mDescriptorHeaps)
        if (heap)
            packed_desc_heaps.push_back(heap);

    mCommandList->SetDescriptorHeaps(UINT(packed_desc_heaps.size()), packed_desc_heaps.data());
}


#endif
