#include "DescriptorAllocator.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

pr::backend::d3d12::DescriptorAllocator::heap::heap(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned size)
    : _type(type), _num_descriptors(size), _num_free_handles(_num_descriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
    heap_desc.Type = _type;
    heap_desc.NumDescriptors = _num_descriptors;

    PR_D3D12_VERIFY(device.CreateDescriptorHeap(&heap_desc, PR_COM_WRITE(_descriptor_heap)));

    _base_descriptor = _descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    _increment_size = device.GetDescriptorHandleIncrementSize(_type);

    add_block(0, _num_free_handles);
}

#endif
