#pragma once

#include <cstdint>

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>

namespace pr::backend::d3d12
{
// In DX12 resource views are represented by handles(called also Descriptors handles). This handles live in a special type of array
// called Descriptor Heap. Placing a few views in contiguously in the same Descriptor Heap allows you
// to create a 'table', that is you can reference the whole table with just a offset(into the descriptor heap)
// and a length. This is a good practice to use tables since the harware runs more efficiently this way.
//
// We need then to allocate arrays of Descriptors into the descriptor heap. The following classes implement a very simple
// linear allocator. Also includes some functions to create Shader/Depth-Stencil/Samples views and assign it to a certain Descriptor.
//
// For every descriptor Heaps there are two types of Descriptor handles, CPU handles an GPU handles.
// To create a view you need a:
//      - resource
//      - a view description structure you need to fill
//      - a CPU handle (lets say the i-th one in your CPU Descritor heap)
//
// In order to bind that resource into the pipeline you'll need to use the i-th handle but from the GPU heap
// this GPU handle is used in SetGraphicsRootDescriptorTable.
//
//
// Since views are represented by just a pair of handles (one for the GPU another for the CPU) we can use a class for all of them.
// Just to avoid mistaking a Sample handle by a Shader Resource, later we'll be creating different types of views.
class ResourceView
{
public:
    uint32_t getSize() const { return mSize; }

    D3D12_CPU_DESCRIPTOR_HANDLE getCPU(uint32_t i = 0) const
    {
        auto res = mHandleCPU;
        res.ptr += i * mDescriptorSize;
        return res;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE getGPU(uint32_t i = 0) const
    {
        auto res = mHandleGPU;
        res.ptr += i * mDescriptorSize;
        return res;
    }

private:
    uint32_t mSize = 0;
    uint32_t mDescriptorSize = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE mHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE mHandleGPU;

    friend class StaticResourceViewHeap;
    void setResourceView(uint32_t size, uint32_t descriptor_size, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
    {
        mSize = size;
        mHandleCPU = cpu;
        mHandleGPU = gpu;
        mDescriptorSize = descriptor_size;
    }
};

// let's add some type safety as mentioned above
class RViewRTV : public ResourceView
{
};
class RViewDSV : public ResourceView
{
};
class RViewCBV_SRV_UAV : public ResourceView
{
};
class RViewSampler : public ResourceView
{
};

// helper class to use a specific type of heap
class StaticResourceViewHeap
{
public:
    void initialize(ID3D12Device& pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorCount);

    bool allocDescriptor(uint32_t size, ResourceView& out_rv)
    {
        if ((mIndex + size) >= mNumDescriptors)
        {
            CC_RUNTIME_ASSERT(false && "StaticResourceViewHeap out of memory, increase its size");
            return false;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE cpu_view = mHeap->GetCPUDescriptorHandleForHeapStart();
        cpu_view.ptr += mIndex * mDescriptorSize;

        D3D12_GPU_DESCRIPTOR_HANDLE gpu_view = mHeap->GetGPUDescriptorHandleForHeapStart();
        gpu_view.ptr += mIndex * mDescriptorSize;

        mIndex += size;

        out_rv.setResourceView(size, mDescriptorSize, cpu_view, gpu_view);

        return true;
    }

    ID3D12DescriptorHeap* getHeap() const { return mHeap; }

private:
    shared_com_ptr<ID3D12DescriptorHeap> mHeap;
    uint32_t mIndex = 0;
    uint32_t mNumDescriptors = 0;
    uint32_t mDescriptorSize = 0;
};

// This class will hold descriptor heaps for all the types of resources. We are going to need them all anyway.
class ResourceViewHeaps
{
public:
    void initialize(ID3D12Device& device, uint32_t num_cbvs, uint32_t num_srvs, uint32_t num_uavs, uint32_t num_dsvs, uint32_t num_rtvs, uint32_t num_samplers)
    {
        mHeapDSVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, num_dsvs);
        mHeapRTVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, num_rtvs);
        mHeapSamplers.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, num_samplers);
        mHeapCBVsSRVsUAVs.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num_cbvs + num_srvs + num_uavs);
    }

    bool allocCBV_SRV_UAV(uint32_t size, RViewCBV_SRV_UAV& out_rv) { return mHeapCBVsSRVsUAVs.allocDescriptor(size, out_rv); }
    bool allocDSV(uint32_t size, RViewDSV& out_rv) { return mHeapDSVs.allocDescriptor(size, out_rv); }
    bool allocRTV(uint32_t size, RViewRTV& out_rv) { return mHeapRTVs.allocDescriptor(size, out_rv); }
    bool allocSampler(uint32_t size, RViewSampler& out_rv) { return mHeapSamplers.allocDescriptor(size, out_rv); }

    ID3D12DescriptorHeap* GetDSVHeap() { return mHeapDSVs.getHeap(); }
    ID3D12DescriptorHeap* GetRTVHeap() { return mHeapRTVs.getHeap(); }
    ID3D12DescriptorHeap* GetSamplerHeap() { return mHeapSamplers.getHeap(); }
    ID3D12DescriptorHeap* GetCBV_SRV_UAVHeap() { return mHeapCBVsSRVsUAVs.getHeap(); }

private:
    StaticResourceViewHeap mHeapDSVs;
    StaticResourceViewHeap mHeapRTVs;
    StaticResourceViewHeap mHeapSamplers;
    StaticResourceViewHeap mHeapCBVsSRVsUAVs;
};
}
