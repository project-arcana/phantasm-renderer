#include "resource_view.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

void pr::backend::d3d12::DescriptorHeap::initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heap_type, uint32_t num_descriptors)
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
    mHeap->SetName(L"DescriptorHeap");
}

pr::backend::d3d12::resource_view pr::backend::d3d12::DescriptorHeap::allocDescriptor(uint32_t size)
{
    if ((mIndex + size) >= mNumDescriptors)
    {
        CC_RUNTIME_ASSERT(false && "StaticResourceViewHeap out of memory, increase its size");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_view = mHeap->GetCPUDescriptorHandleForHeapStart();
    cpu_view.ptr += mIndex * mDescriptorSize;

    D3D12_GPU_DESCRIPTOR_HANDLE gpu_view = mHeap->GetGPUDescriptorHandleForHeapStart();
    gpu_view.ptr += mIndex * mDescriptorSize;

    mIndex += size;

    return resource_view(size, mDescriptorSize, cpu_view, gpu_view);
}

void pr::backend::d3d12::DescriptorHeap::reset() { mIndex = 0; }

void pr::backend::d3d12::ResourceViewAllocator::initialize(
    ID3D12Device& device, uint32_t num_cbvs, uint32_t num_srvs, uint32_t num_uavs, uint32_t num_dsvs, uint32_t num_rtvs, uint32_t num_samplers)
{
    mHeapDSVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, num_dsvs);
    mHeapRTVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, num_rtvs);
    mHeapSamplers.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, num_samplers);
    mHeapCBVsSRVsUAVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num_cbvs + num_srvs + num_uavs);
}

void pr::backend::d3d12::ResourceViewAllocator::resetAll()
{
    mHeapDSVs.reset();
    mHeapRTVs.reset();
    mHeapSamplers.reset();
    mHeapCBVsSRVsUAVs.reset();
}

void pr::backend::d3d12::StaticDescriptorPool::initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned size)
{
    auto const desc_size = device.GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = size;
    desc.Type = type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;
    PR_D3D12_VERIFY(device.CreateDescriptorHeap(&desc, PR_COM_WRITE(mHeap)));
    mHeap->SetName(L"StaticDescriptorPool::mHeap");

    auto const heap_start = mHeap->GetCPUDescriptorHandleForHeapStart().ptr;

    mFreeList.reserve(size);
    for (auto i = 0u; i < size; ++i)
    {
        mFreeList.push_back(D3D12_CPU_DESCRIPTOR_HANDLE{heap_start + i * desc_size});
    }
}

void pr::backend::d3d12::DynamicDescriptorRing::initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned size, unsigned num_frames)
{
    CC_RUNTIME_ASSERT(!((type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) || (type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)) && "Only use this class for GPU-visible descriptors");

    mRing.initialize(num_frames, size);
    mDescriptorSize = device.GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = size;
    desc.Type = type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    PR_D3D12_VERIFY(device.CreateDescriptorHeap(&desc, PR_COM_WRITE(mHeap)));
    mHeap->SetName(L"DynamicDescriptorRing::mHeap");

    mHandleCPU = mHeap->GetCPUDescriptorHandleForHeapStart();
    mHandleGPU = mHeap->GetGPUDescriptorHandleForHeapStart();
}

pr::backend::d3d12::resource_view pr::backend::d3d12::DynamicDescriptorRing::allocate(unsigned num)
{
    unsigned index;
    bool const success = mRing.alloc(num, &index);
    CC_RUNTIME_ASSERT(success && "Dynamic descriptor ring full");

    return resource_view(num, mDescriptorSize, D3D12_CPU_DESCRIPTOR_HANDLE{mHandleCPU.ptr + index * mDescriptorSize},
                         D3D12_GPU_DESCRIPTOR_HANDLE{mHandleGPU.ptr + index * mDescriptorSize});
}

void pr::backend::d3d12::DescriptorManager::initialize(ID3D12Device& device, uint32_t num_cbv_srv_uav, uint32_t num_dsv, uint32_t num_rtv, uint32_t num_sampler, uint32_t num_backbuffers)
{
    mPoolDSVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, num_dsv);
    mPoolRTVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, num_rtv);
    mPoolSamplers.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, num_sampler);
    mPoolCBVsSRVsUAVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num_cbv_srv_uav);

    mRingCBVsSRVsUAVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num_cbv_srv_uav, num_backbuffers);
    mRingSamplers.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, num_sampler, num_backbuffers);
}
