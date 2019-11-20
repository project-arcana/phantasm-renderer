#pragma once

#include <mutex>

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/shared_com_ptr.hh>
#include <phantasm-renderer/backend/detail/page_allocator.hh>
#include <phantasm-renderer/backend/types.hh>

namespace pr::backend::d3d12
{
/// A page allocator for variable-sized, GPU-visible descriptors
/// Currently unused, but planned to be the main descriptor allocator used throughout the application
///
/// Descriptors are only used for shader arguments, and play two roles there:
///     - Single CBV root descriptor
///         This one should ideally come from a different, freelist allocator since by nature its always of size 1
///     - Shader view
///          n contiguous SRVs and m contiguous UAVs
///         This allocator is intended for this scenario
///         We likely do not want to keep additional descriptors around,
///         Just allocate here once and directly device.make... the descriptors in-place
///         As both are the same type, we just need a single of these allocators
///
/// We might have to add defragmentation at some point, which would probably require an additional indirection
/// Lookup and free is O(1), allocate is O(#pages), but still fast and skipping blocks
class DescriptorPageAllocator
{
public:
    using handle_t = int;

public:
    void initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, int num_descriptors, int page_size = 8);

    [[nodiscard]] handle_t allocate(int num_descriptors)
    {
        auto const res_page = mPageAllocator.allocate(num_descriptors);
        CC_RUNTIME_ASSERT(res_page != -1 && "descriptor_page_allocator overcommitted!");
        return res_page;
    }

    void free(handle_t handle) { mPageAllocator.free(handle); }

public:
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE getCPUStart(handle_t handle) const
    {
        // index = page index * page size
        auto const index = handle * mPageAllocator.get_page_size();
        return D3D12_CPU_DESCRIPTOR_HANDLE{mHeapStartCPU.ptr + unsigned(index) * mDescriptorSize};
    }

    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE getGPUStart(handle_t handle) const
    {
        // index = page index * page size
        auto const index = handle * mPageAllocator.get_page_size();
        return D3D12_GPU_DESCRIPTOR_HANDLE{mHeapStartGPU.ptr + unsigned(index) * mDescriptorSize};
    }

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE incrementToIndex(D3D12_CPU_DESCRIPTOR_HANDLE desc, unsigned i) const
    {
        desc.ptr += i * mDescriptorSize;
        return desc;
    }

    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE incrementToIndex(D3D12_GPU_DESCRIPTOR_HANDLE desc, unsigned i) const
    {
        desc.ptr += i * mDescriptorSize;
        return desc;
    }

    [[nodiscard]] int getNumPages() const { return mPageAllocator.get_num_pages(); }

private:
    shared_com_ptr<ID3D12DescriptorHeap> mHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE mHeapStartCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE mHeapStartGPU;
    backend::detail::page_allocator mPageAllocator;
    unsigned mDescriptorSize = 0;
};

class ResourcePool;

/// The high-level allocator for shader views
/// Synchronized
class ShaderViewPool
{
public:
    // frontend-facing API

    [[nodiscard]] handle::shader_view create(cc::span<handle::resource> srvs, cc::span<handle::resource> uavs);

    void free(handle::shader_view sv)
    {
        auto lg = std::lock_guard(mMutex);
        mSRVUAVAllocator.free(sv.index);
    }

public:
    // internal API

    void initialize(ID3D12Device* device, ResourcePool* res_pool, int num_srvs_uavs)
    {
        mDevice = device;
        mResourcePool = res_pool;
        mSRVUAVAllocator.initialize(*device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num_srvs_uavs);
        mShaderViewData = cc::array<shader_view_data>::uninitialized(unsigned(mSRVUAVAllocator.getNumPages()));
    }

    // NOTE: is CPU even required?
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE getCPUStart(handle::shader_view sv) const { return mSRVUAVAllocator.getCPUStart(sv.index); }

    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE getGPUStart(handle::shader_view sv) const
    {
        // cached fastpath
        return mShaderViewData[static_cast<size_t>(sv.index)].gpu_handle;
    }

    //
    // Receive contained resource handles for state management
    //

    [[nodiscard]] cc::span<handle::resource const> getSRVs(handle::shader_view sv) const
    {
        auto const& data = mShaderViewData[static_cast<size_t>(sv.index)];
        return cc::span{data.resources.data(), static_cast<size_t>(data.num_srvs)};
    }

    [[nodiscard]] cc::span<handle::resource const> getUAVs(handle::shader_view sv) const
    {
        auto const& data = mShaderViewData[static_cast<size_t>(sv.index)];
        return cc::span{data.resources.data() + data.num_srvs, static_cast<size_t>(data.num_uavs)};
    }

private:
    struct shader_view_data
    {
        // pre-constructed gpu handle
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        // handles contained in this shader view, for state tracking
        // handles correspond to num_srvs SRVs first, num_uavs UAVs second
        cc::capped_vector<handle::resource, 16> resources;
        cc::uint16 num_srvs;
        cc::uint16 num_uavs;
    };

    // non-owning
    ID3D12Device* mDevice;
    ResourcePool* mResourcePool;

    cc::array<shader_view_data> mShaderViewData;
    DescriptorPageAllocator mSRVUAVAllocator;
    std::mutex mMutex;
};
}