#pragma once

#include <cstdint>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>

namespace pr::backend::d3d12
{
struct resource_view
{
    resource_view(unsigned size, unsigned desc_size, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
      : size(size), _descriptor_size(desc_size), _handle_cpu(cpu), _handle_gpu(gpu)
    {
    }

    unsigned size = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE get_cpu(unsigned i = 0) const
    {
        return D3D12_CPU_DESCRIPTOR_HANDLE{ _handle_cpu.ptr + i * _descriptor_size };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE get_gpu(unsigned i = 0) const
    {
        return D3D12_GPU_DESCRIPTOR_HANDLE{ _handle_gpu.ptr + i * _descriptor_size };
    }

private:
    unsigned _descriptor_size = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE _handle_cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE _handle_gpu;
};

class DescriptorHeap
{
public:
    void initialize(ID3D12Device& pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorCount);

    /// Allocates a resource view of the given size
    [[nodiscard]] resource_view allocDescriptor(uint32_t size);

    /// Resets the underlying linear allocator, invalidating all previous resource views
    void reset();

    ID3D12DescriptorHeap* getHeap() const { return mHeap; }

private:
    shared_com_ptr<ID3D12DescriptorHeap> mHeap;
    uint32_t mIndex = 0;
    uint32_t mNumDescriptors = 0;
    uint32_t mDescriptorSize = 0;
};

class ResourceViewAllocator
{
public:
    void initialize(ID3D12Device& device, uint32_t num_cbvs, uint32_t num_srvs, uint32_t num_uavs, uint32_t num_dsvs, uint32_t num_rtvs, uint32_t num_samplers);

    [[nodiscard]] resource_view allocCBV_SRV_UAV(uint32_t size) { return mHeapCBVsSRVsUAVs.allocDescriptor(size); }
    [[nodiscard]] resource_view allocDSV(uint32_t size) { return mHeapDSVs.allocDescriptor(size); }
    [[nodiscard]] resource_view allocRTV(uint32_t size) { return mHeapRTVs.allocDescriptor(size); }
    [[nodiscard]] resource_view allocSampler(uint32_t size) { return mHeapSamplers.allocDescriptor(size); }

    ID3D12DescriptorHeap* getDSVHeap() { return mHeapDSVs.getHeap(); }
    ID3D12DescriptorHeap* getRTVHeap() { return mHeapRTVs.getHeap(); }
    ID3D12DescriptorHeap* getSamplerHeap() { return mHeapSamplers.getHeap(); }
    ID3D12DescriptorHeap* getCBV_SRV_UAVHeap() { return mHeapCBVsSRVsUAVs.getHeap(); }

    /// Resets all underlying linear allocators, invalidating all previous resource views
    void resetAll();

private:
    DescriptorHeap mHeapDSVs;
    DescriptorHeap mHeapRTVs;
    DescriptorHeap mHeapSamplers;
    DescriptorHeap mHeapCBVsSRVsUAVs;
};
}
