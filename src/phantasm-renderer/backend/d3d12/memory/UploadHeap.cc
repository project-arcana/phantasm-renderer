#include "UploadHeap.hh"

#include <phantasm-renderer/backend/d3d12/BackendD3D12.hh>
#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/util.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>
#include <phantasm-renderer/backend/d3d12/memory/Allocator.hh>

#include "byte_util.hh"

namespace pr::backend::d3d12
{
void UploadHeap::initialize(BackendD3D12* backend, size_t size)
{
    mBackend = backend;
    // Create command list and allocators

    auto& device = mBackend->mDevice.getDevice();
    device.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, PR_COM_WRITE(mCommandAllocator));
    mCommandAllocator->SetName(L"UploadHeap::mCommandAllocator");
    device.CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, nullptr, PR_COM_WRITE(mCommandList));
    mCommandList->SetName(L"UploadHeap::mCommandList");

    // Create buffer to suballocate
    {
        auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto const resource_desc = CD3DX12_RESOURCE_DESC::Buffer(size);
        PR_D3D12_VERIFY(device.CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                       PR_COM_WRITE(mUploadHeap)));

        PR_D3D12_VERIFY(mUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&mDataBegin)));
    }

    mDataCurrent = mDataBegin;
    mDataEnd = mDataBegin + mUploadHeap->GetDesc().Width;
    mPendingInitBarriers.reserve(16);
}

uint8_t* UploadHeap::suballocate(size_t size, size_t align)
{
    mDataCurrent = reinterpret_cast<uint8_t*>(mem::align_offset(reinterpret_cast<size_t>(mDataCurrent), size_t(align)));

    // return NULL if we ran out of space in the heap
    if (mDataCurrent >= mDataEnd || mDataCurrent + size >= mDataEnd)
    {
        return nullptr;
    }

    auto const res = mDataCurrent;
    mDataCurrent += size;
    return res;
}

uint8_t* UploadHeap::suballocateAllowRetry(size_t size, size_t alignment)
{
    auto res = suballocate(size, alignment);
    if (res == nullptr)
    {
        flushAndFinish();
        res = suballocate(size, alignment);
        CC_ASSERT(res != nullptr);
    }
    return res;
}

void UploadHeap::copyAllocationToBuffer(ID3D12Resource* dest_resource, uint8_t* src_allocation, size_t size)
{
    auto offset = src_allocation - mDataBegin;
    CC_ASSERT(offset >= 0 && offset <= (mDataEnd - mDataBegin) && "allocation not in range");
    CC_ASSERT(src_allocation + size <= mDataEnd && "allocation copy out of bounds");
    mCommandList->CopyBufferRegion(dest_resource, 0, mUploadHeap, UINT64(offset), size);
}

void UploadHeap::barrierResourceOnFlush(D3D12MA::Allocation* allocation, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    mPendingInitBarriers.emplace_back(allocation, before, after);
}

//--------------------------------------------------------------------------------------
//
// FlushAndFinish
//
//--------------------------------------------------------------------------------------
void UploadHeap::flushAndFinish()
{
    if (!mPendingInitBarriers.empty())
    {
        master_state_cache::submit_initial_creation_barriers(mCommandList, mPendingInitBarriers);
        mPendingInitBarriers.clear();
    }

    // Close & submit
    PR_D3D12_VERIFY(mCommandList->Close());
    auto const command_list_raw = mCommandList.get();
    mBackend->mDirectQueue.getQueue().ExecuteCommandLists(1, CommandListCast(&command_list_raw));

    // Make sure it's been processed by the GPU
    mBackend->flushGPU();

    // Reset so it can be reused
    mCommandAllocator->Reset();
    mCommandList->Reset(mCommandAllocator, nullptr);

    mDataCurrent = mDataBegin;
}
}
