#include "DescriptorAllocator.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/assert.hh>
#include <clean-core/move.hh>
#include <clean-core/utility.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

pr::backend::d3d12::DescriptorAllocator::allocation::allocation(D3D12_CPU_DESCRIPTOR_HANDLE handle, int num_handles, int descriptor_size, DescriptorAllocator::heap* parent)
  : _parent_heap(parent), _handle(handle), _num_handles(num_handles), _handle_size(descriptor_size)
{
}

pr::backend::d3d12::DescriptorAllocator::allocation::allocation(pr::backend::d3d12::DescriptorAllocator::allocation&& rhs) noexcept
  : _parent_heap(rhs._parent_heap), _handle(rhs._handle), _num_handles(rhs._num_handles), _handle_size(rhs._handle_size)
{
    rhs.invalidate();
}

pr::backend::d3d12::DescriptorAllocator::allocation& pr::backend::d3d12::DescriptorAllocator::allocation::operator=(pr::backend::d3d12::DescriptorAllocator::allocation&& rhs) noexcept
{
    this->free();

    _parent_heap = rhs._parent_heap;
    _handle = rhs._handle;
    _num_handles = rhs._num_handles;
    _handle_size = rhs._handle_size;

    rhs.invalidate();

    return *this;
}

pr::backend::d3d12::DescriptorAllocator::allocation::~allocation() { this->free(); }

void pr::backend::d3d12::DescriptorAllocator::allocation::free()
{
    if (is_valid())
    {
        CC_ASSERT(_parent_heap != nullptr);
        _parent_heap->free(cc::move(*this), 0u); // TODO: framegen
        invalidate();
    }
}

void pr::backend::d3d12::DescriptorAllocator::allocation::invalidate() { _handle.ptr = 0; }

#endif
