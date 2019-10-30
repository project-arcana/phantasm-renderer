#include "DescriptorAllocator.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

pr::backend::d3d12::DescriptorAllocator::DescriptorAllocator(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned heap_size)
  : mDevice(device), mType(type), mHeapSize(heap_size)
{
}

pr::backend::d3d12::DescriptorAllocator::allocation pr::backend::d3d12::DescriptorAllocator::allocate(unsigned num_descriptors)
{
    std::lock_guard lg(mMutex);

    allocation res;

    for (auto it = mFreeHeapIndices.begin(); it != mFreeHeapIndices.end(); ++it)
    {
        auto const& heap = mPool[*it];
        res = heap->allocate(num_descriptors);

        if (heap->get_num_free_handles() == 0)
        {
            it = mFreeHeapIndices.erase(it);
        }

        if (res.is_valid())
            return res;
    }

    // No available heap could allocate, create a new one
    mHeapSize = cc::max(mHeapSize, num_descriptors);
    auto const new_heap = createHeap();
    res = new_heap->allocate(num_descriptors);

    return res;
}

void pr::backend::d3d12::DescriptorAllocator::cleanUp(cc::uint64 frame_generation)
{
    std::lock_guard lg(mMutex);

    for (auto i = 0u; i < mPool.size(); ++i)
    {
        auto const& heap = mPool[i];

        heap->clean_up(frame_generation);
        if (heap->get_num_free_handles() > 0)
            mFreeHeapIndices.insert(i);
    }
}

pr::backend::d3d12::DescriptorAllocator::shared_heap_t pr::backend::d3d12::DescriptorAllocator::createHeap()
{
    auto res = std::make_shared<heap>(mDevice, mType, mHeapSize);
    mPool.push_back(res);
    mFreeHeapIndices.insert(mPool.size() - 1);
    return res;
}


#endif
