#include "ResourceViewHeaps.hh"
#ifdef PR_BACKEND_D3D12

#include "common/verify.hh"

namespace pr::backend::d3d12
{
void StaticResourceViewHeap::initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32_t num_descriptors)
{
    mNumDescriptors = num_descriptors;
    mIndex = 0;
    mDescriptorSize = device.GetDescriptorHandleIncrementSize(heap_type);

    D3D12_DESCRIPTOR_HEAP_DESC descHeap;
    descHeap.NumDescriptors = num_descriptors;
    descHeap.Type = heap_type;
    descHeap.Flags = ((heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) || (heap_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV))
                         ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE
                         : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descHeap.NodeMask = 0;
    PR_D3D12_VERIFY(device.CreateDescriptorHeap(&descHeap, PR_COM_WRITE(mHeap)));
    mHeap->SetName(L"StaticHeapDX12");
}
}

#endif
