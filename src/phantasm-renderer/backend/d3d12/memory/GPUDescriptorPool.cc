#include "GPUDescriptorPool.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/d3d12/RootSignature.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

pr::backend::d3d12::GPUDescriptorPool::GPUDescriptorPool(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, int numDescriptorsPerHeap)
  : mDevice(device),
    mDescriptorHeapType(heapType),
    mNumDescriptorsPerHeap(numDescriptorsPerHeap),
    mDescriptorHandleIncrementSize(mDevice.GetDescriptorHandleIncrementSize(mDescriptorHeapType)),
    // Allocate space for staging CPU visible descriptors
    mDescriptorHandleCache(cc::array<D3D12_CPU_DESCRIPTOR_HANDLE>::uninitialized(cc::size_t(mNumDescriptorsPerHeap)))
{
    CC_ASSERT(numDescriptorsPerHeap > 0);
}

pr::backend::d3d12::GPUDescriptorPool::~GPUDescriptorPool()
{
    for (auto const& heap : mDescriptorHeapPool)
        heap->Release();
}

void pr::backend::d3d12::GPUDescriptorPool::parseRootSignature(RootSignature const& rootSignature)
{
    // If the root signature changes, all descriptors must be (re)bound to the
    // command list.
    mStaleDescriptorTableBitMask = 0;

    auto const root_sig_num_params = rootSignature.getDescription().NumParameters;

    // Get a bit mask that represents the root parameter indices that match the
    // descriptor heap type for this dynamic descriptor heap.
    auto desc_table_bit_mask = rootSignature.getBitmask(mDescriptorHeapType);
    mDescriptorTableBitMask = desc_table_bit_mask;

    auto currentOffset = 0;
    DWORD root_index;
    while (_BitScanForward(&root_index, desc_table_bit_mask) && root_index < root_sig_num_params)
    {
        auto const numDescriptors = rootSignature.getNumDescriptors(root_index);

        descriptor_table_cache& descriptorTableCache = mDescriptorTableCache[root_index];
        descriptorTableCache.size = int(numDescriptors);
        descriptorTableCache.base_descriptor = mDescriptorHandleCache.data() + currentOffset;

        currentOffset += numDescriptors;

        // Flip the descriptor table bit so it's not scanned again for the current index.
        desc_table_bit_mask ^= (1 << root_index);
    }

    // Make sure the maximum number of descriptors per descriptor heap has not been exceeded.
    CC_ASSERT(currentOffset <= mNumDescriptorsPerHeap
              && "The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum "
                 "number "
                 "of descriptors per descriptor heap.");
}

void pr::backend::d3d12::GPUDescriptorPool::stageDescriptors(unsigned rootParameterIndex, int offset, int numDescriptors, D3D12_CPU_DESCRIPTOR_HANDLE src_descriptor)
{
    // Cannot stage more than the maximum number of descriptors per heap.
    // Cannot stage more than max_descriptor_tables root parameters.
    CC_RUNTIME_ASSERT(numDescriptors <= mNumDescriptorsPerHeap && rootParameterIndex < max_descriptor_tables);

    auto const& descriptorTableCache = mDescriptorTableCache[rootParameterIndex];

    // Check that the number of descriptors to copy does not exceed the number
    // of descriptors expected in the descriptor table.
    CC_RUNTIME_ASSERT((offset + numDescriptors) <= descriptorTableCache.size && "Number of descriptors exceeds the number of descriptors in the descriptor table.");

    D3D12_CPU_DESCRIPTOR_HANDLE* const dest_descriptor = (descriptorTableCache.base_descriptor + offset);
    for (auto i = 0; i < numDescriptors; ++i)
    {
        dest_descriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(src_descriptor, i, mDescriptorHandleIncrementSize);
    }

    // Set the root parameter index bit to make sure the descriptor table
    // at that index is bound to the command list.
    mStaleDescriptorTableBitMask |= (1 << rootParameterIndex);
}

int pr::backend::d3d12::GPUDescriptorPool::getNumStaleDescriptors() const
{
    auto res = 0;
    DWORD i;
    DWORD bit_mask = mStaleDescriptorTableBitMask;
    while (_BitScanForward(&i, bit_mask))
    {
        res += mDescriptorTableCache[i].size;
        bit_mask ^= (1 << i);
    }

    return res;
}

bool pr::backend::d3d12::GPUDescriptorPool::prepareDescriptorCommit(CommandList& commandList)
{
    // Compute the number of descriptors that need to be copied
    auto const num_commitable_descr = getNumStaleDescriptors();

    if (num_commitable_descr > 0)
    {
        if (!mCurrentDescriptorHeap || mNumFreeHandles < num_commitable_descr)
        {
            mCurrentDescriptorHeap = acquireHeap();
            mCurrentCPUDescriptorHandle = mCurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            mCurrentGPUDescriptorHandle = mCurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
            mNumFreeHandles = mNumDescriptorsPerHeap;

            commandList.setDescriptorHeap(mDescriptorHeapType, mCurrentDescriptorHeap);

            // When updating the descriptor heap on the command list, all descriptor
            // tables must be (re)recopied to the new descriptor heap (not just
            // the stale descriptor tables).
            mStaleDescriptorTableBitMask = mDescriptorTableBitMask;
        }

        return true;
    }

    return false;
}

ID3D12DescriptorHeap* pr::backend::d3d12::GPUDescriptorPool::acquireHeap()
{
    if (!mAvailableDescriptorHeaps.empty())
    {
        auto const res = mAvailableDescriptorHeaps.back();
        mAvailableDescriptorHeaps.pop_back();
        return res;
    }
    else
    {
        D3D12_DESCRIPTOR_HEAP_DESC dheap_desc = {};
        dheap_desc.Type = mDescriptorHeapType;
        dheap_desc.NumDescriptors = UINT(mNumDescriptorsPerHeap);
        dheap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        ID3D12DescriptorHeap*& new_heap = mDescriptorHeapPool.emplace_back();
        PR_D3D12_VERIFY(mDevice.CreateDescriptorHeap(&dheap_desc, IID_PPV_ARGS(&new_heap)));
        return new_heap;
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE pr::backend::d3d12::GPUDescriptorPool::copyDescriptor(CommandList& commandList, D3D12_CPU_DESCRIPTOR_HANDLE cpu_descriptor)
{
    if (!mCurrentDescriptorHeap || mNumFreeHandles < 1)
    {
        mCurrentDescriptorHeap = acquireHeap();
        mCurrentCPUDescriptorHandle = mCurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        mCurrentGPUDescriptorHandle = mCurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        mNumFreeHandles = mNumDescriptorsPerHeap;

        commandList.setDescriptorHeap(mDescriptorHeapType, mCurrentDescriptorHeap);

        // When updating the descriptor heap on the command list, all descriptor
        // tables must be (re)recopied to the new descriptor heap (not just
        // the stale descriptor tables).
        mStaleDescriptorTableBitMask = mDescriptorTableBitMask;
    }

    auto const res = mCurrentGPUDescriptorHandle;

    mDevice.CopyDescriptorsSimple(1, mCurrentCPUDescriptorHandle, cpu_descriptor, mDescriptorHeapType);
    mCurrentCPUDescriptorHandle.Offset(1, mDescriptorHandleIncrementSize);
    mCurrentGPUDescriptorHandle.Offset(1, mDescriptorHandleIncrementSize);
    --mNumFreeHandles;

    return res;
}

void pr::backend::d3d12::GPUDescriptorPool::reset()
{
    mAvailableDescriptorHeaps = mDescriptorHeapPool;
    mCurrentDescriptorHeap = nullptr;
    mCurrentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    mCurrentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
    mNumFreeHandles = 0;
    mDescriptorTableBitMask = 0;
    mStaleDescriptorTableBitMask = 0;

    // Reset the table cache
    for (auto& table_cache : mDescriptorTableCache)
        table_cache.reset();
}
#endif
