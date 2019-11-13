#include "resource.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/d3d12/common/verify.hh>
#include <phantasm-renderer/backend/d3d12/memory/Allocator.hh>

void pr::backend::d3d12::ResourceAllocator::initialize(ID3D12Device& device)
{
    CC_ASSERT(mAllocator == nullptr);

    D3D12MA::ALLOCATOR_DESC allocator_desc = {};
    allocator_desc.Flags = D3D12MA::ALLOCATOR_FLAGS::ALLOCATOR_FLAG_NONE;
    allocator_desc.pDevice = &device;
    allocator_desc.PreferredBlockSize = 0;         // default
    allocator_desc.pAllocationCallbacks = nullptr; // default

    auto const hr = D3D12MA::CreateAllocator(&allocator_desc, &mAllocator);
    PR_D3D12_ASSERT(hr);
}

pr::backend::d3d12::ResourceAllocator::~ResourceAllocator()
{
    // this is not a COM pointer although it looks like one
    if (mAllocator)
        mAllocator->Release();
}

pr::backend::d3d12::resource pr::backend::d3d12::ResourceAllocator::allocateResource(const D3D12_RESOURCE_DESC& desc,
                                                                                     D3D12_RESOURCE_STATES initial_state,
                                                                                     D3D12_CLEAR_VALUE* clear_value,
                                                                                     D3D12_HEAP_TYPE heap_type) const
{
    D3D12MA::ALLOCATION_DESC allocation_desc = {};
    allocation_desc.Flags = D3D12MA::ALLOCATION_FLAG_NONE;
    allocation_desc.HeapType = heap_type;

    resource res;
    auto const hr = mAllocator->CreateResource(&allocation_desc, &desc, initial_state, clear_value, &res._allocation, __uuidof(ID3D12Resource), nullptr);
    PR_D3D12_ASSERT(hr);
    res.raw = res._allocation->GetResource();
    res._allocation->pr_setResourceState(initial_state);
    return res;
}

pr::backend::d3d12::resource::~resource() { free(); }

void pr::backend::d3d12::resource::free()
{
    if (_allocation)
    {
        _allocation->Release();
        _allocation = nullptr;
    }
}
