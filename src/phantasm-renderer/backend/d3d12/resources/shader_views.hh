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
class descriptor_page_allocator
{
public:
    using handle_t = int;

public:
    void initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, int num_descriptors, int page_size = 8);

    [[nodiscard]] handle_t allocate(int num_descriptors)
    {
        auto const res_page = _page_allocator.allocate(num_descriptors);
        CC_RUNTIME_ASSERT(res_page != -1 && "descriptor_page_allocator overcommitted!");
        return res_page;
    }

    void free(handle_t handle) { _page_allocator.free(handle); }

public:
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_start(handle_t handle) const
    {
        // index = page index * page size
        auto const index = handle * _page_allocator.get_page_size();
        return D3D12_CPU_DESCRIPTOR_HANDLE{_heap_start_cpu.ptr + unsigned(index) * _descriptor_size};
    }

    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_start(handle_t handle) const
    {
        // index = page index * page size
        auto const index = handle * _page_allocator.get_page_size();
        return D3D12_GPU_DESCRIPTOR_HANDLE{_heap_start_gpu.ptr + unsigned(index) * _descriptor_size};
    }

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE increment_to_index(D3D12_CPU_DESCRIPTOR_HANDLE desc, unsigned i) const
    {
        desc.ptr += i * _descriptor_size;
        return desc;
    }

    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE increment_to_index(D3D12_GPU_DESCRIPTOR_HANDLE desc, unsigned i) const
    {
        desc.ptr += i * _descriptor_size;
        return desc;
    }

    [[nodiscard]] int get_num_pages() const { return _page_allocator.get_num_pages(); }

private:
    shared_com_ptr<ID3D12DescriptorHeap> _heap;
    D3D12_CPU_DESCRIPTOR_HANDLE _heap_start_cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE _heap_start_gpu;
    backend::detail::page_allocator _page_allocator;
    unsigned _descriptor_size = 0;
};

/// the high-level, synchronized allocator for shader views
class shader_view_allocator
{
public:
    // backend-facing API
    [[nodiscard]] handle::shader_view create(ID3D12Device& device, cc::span<handle::resource> srvs, cc::span<handle::resource> uavs);

    void destroy(handle::shader_view sv)
    {
        std::lock_guard lg(_mutex);
        _srv_uav_allocator.free(sv.index);
    }

public:
    // internal API
    void initialize(ID3D12Device& device, int num_srvs_uavs)
    {
        _srv_uav_allocator.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num_srvs_uavs);
        _shader_view_data = cc::array<shader_view_data>::uninitialized(unsigned(_srv_uav_allocator.get_num_pages()));
    }

    /// NOTE: is CPU even required?
    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_start(handle::shader_view sv) const { return _srv_uav_allocator.get_cpu_start(sv.index); }

    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_start(handle::shader_view sv) const { return _srv_uav_allocator.get_gpu_start(sv.index); }

private:
    struct shader_view_data
    {
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        // we must store the resource handles contained in this shader view for state tracking
        cc::capped_vector<handle::resource, 16> resources;
        cc::uint16 num_srvs;
        cc::uint16 num_uavs;
    };
    cc::array<shader_view_data> _shader_view_data;
    descriptor_page_allocator _srv_uav_allocator;
    std::mutex _mutex;
};
}
