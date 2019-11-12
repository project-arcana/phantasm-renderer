#pragma once

#include <cstdint>

#include <clean-core/span.hh>
#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
#include <phantasm-renderer/backend/d3d12/memory/Ring.hh>

namespace pr::backend::d3d12
{
struct resource_view
{
    resource_view() = default;
    resource_view(unsigned size, unsigned desc_size, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
      : size(size), _descriptor_size(desc_size), _handle_cpu(cpu), _handle_gpu(gpu)
    {
    }

    unsigned size = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE get_cpu(unsigned i = 0) const { return D3D12_CPU_DESCRIPTOR_HANDLE{_handle_cpu.ptr + i * _descriptor_size}; }

    D3D12_GPU_DESCRIPTOR_HANDLE get_gpu(unsigned i = 0) const { return D3D12_GPU_DESCRIPTOR_HANDLE{_handle_gpu.ptr + i * _descriptor_size}; }

private:
    unsigned _descriptor_size = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE _handle_cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE _handle_gpu;
};

/// A non-GPU-visible, single descriptor handle for a static resource
struct cpu_resource_view
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
};

struct cpu_cbv_srv_uav : cpu_resource_view
{
};

struct cpu_sampler : cpu_resource_view
{
    // this is NOT a static sampler, its a descriptor handle to a heap sampler
};

struct cpu_dsv : cpu_resource_view
{
};

struct cpu_rtv : cpu_resource_view
{
};

struct dynamic_descriptor_table
{
    D3D12_GPU_DESCRIPTOR_HANDLE shader_resource_handle_gpu; ///< GPU handle to the base descriptor for CBVs, SRVs and UAVs
};

/// A non-GPU-visible pool for long-lived descriptors of size 1
/// Should be copied contiguously into a DynamicDescriptorRing at dispatch time
class CPUDescriptorPool
{
public:
    void initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned size);

    /// allocates a single descriptor of size 1
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE allocate()
    {
        CC_ASSERT(!mFreeList.empty());
        auto res = mFreeList.back();
        mFreeList.pop_back();
        return res;
    }

    /// frees a descriptor previously received from this pool
    void free(D3D12_CPU_DESCRIPTOR_HANDLE const& handle) { mFreeList.push_back(handle); }

    /// returns the amount of free descriptors in this pool
    size_t numFreeDescriptors() const { return mFreeList.size(); }

private:
    shared_com_ptr<ID3D12DescriptorHeap> mHeap;
    cc::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mFreeList;
};

/// A GPU-visible ring buffer for frame-local descriptors,
/// meant to be copied to from statically existing ones
class GPUDescriptorRing
{
public:
    void initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned size, unsigned num_frames);

    [[nodiscard]] resource_view allocate(unsigned num);

    void onBeginFrame() { mRing.onBeginFrame(); }

    ID3D12DescriptorHeap* getHeap() const { return mHeap; }

private:
    shared_com_ptr<ID3D12DescriptorHeap> mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE mHandleCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE mHandleGPU;
    RingWithTabs mRing;
    unsigned mDescriptorSize = 0;
};

class DescriptorAllocator
{
public:
    void initialize(ID3D12Device& device, uint32_t num_cbv_srv_uav, uint32_t num_dsv, uint32_t num_rtv, uint32_t num_sampler, uint32_t num_backbuffers);

    [[nodiscard]] cpu_cbv_srv_uav allocCBV_SRV_UAV() { return {{mPoolCBVsSRVsUAVs.allocate()}}; }
    [[nodiscard]] cpu_sampler allocSampler() { return {{mPoolSamplers.allocate()}}; }
    [[nodiscard]] cpu_rtv allocRTV() { return {{mPoolRTVs.allocate()}}; }
    [[nodiscard]] cpu_dsv allocDSV() { return {{mPoolDSVs.allocate()}}; }

    void free(cpu_cbv_srv_uav const& static_view) { mPoolCBVsSRVsUAVs.free(static_view.handle); }
    void free(cpu_sampler const& static_view) { mPoolSamplers.free(static_view.handle); }
    void free(cpu_rtv const& static_view) { mPoolRTVs.free(static_view.handle); }
    void free(cpu_dsv const& static_view) { mPoolDSVs.free(static_view.handle); }

    /// allocates a contiguous, GPU-visible, frame-local descriptor for the given range of CBVs/SRVs/UAVs
    /// for use in descriptor tables (TODO: heap samplers)
    [[nodiscard]] dynamic_descriptor_table allocDynamicTable(ID3D12Device& device, cc::span<cpu_cbv_srv_uav const> resources)
    {
        auto const num = unsigned(resources.size());
        auto handle = mRingCBVsSRVsUAVs.allocate(num);

        for (auto i = 0u; i < num; ++i)
        {
            device.CopyDescriptorsSimple(1, handle.get_cpu(i), resources[i].handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        return {handle.get_gpu()};
    }

    void onBeginFrame()
    {
        mRingCBVsSRVsUAVs.onBeginFrame();
        mRingSamplers.onBeginFrame();
    }

    ID3D12DescriptorHeap* getSamplerHeap() const { return mRingSamplers.getHeap(); }
    ID3D12DescriptorHeap* getCBV_SRV_UAVHeap() const { return mRingCBVsSRVsUAVs.getHeap(); }

private:
    CPUDescriptorPool mPoolCBVsSRVsUAVs;
    CPUDescriptorPool mPoolSamplers;
    CPUDescriptorPool mPoolRTVs;
    CPUDescriptorPool mPoolDSVs;

    GPUDescriptorRing mRingCBVsSRVsUAVs;
    GPUDescriptorRing mRingSamplers;
};
}
