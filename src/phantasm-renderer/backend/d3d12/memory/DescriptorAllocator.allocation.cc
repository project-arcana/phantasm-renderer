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

void pr::backend::d3d12::DescriptorAllocator::allocation::free(cc::uint64 current_frame_gen)
{
    CC_ASSERT(_parent_heap != nullptr);
    _parent_heap->free(cc::move(*this), current_frame_gen);
}

#endif
