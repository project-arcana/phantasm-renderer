#include "shader_views.hh"

#include <phantasm-renderer/backend/d3d12/common/verify.hh>

#include "resource_creation.hh"

void pr::backend::d3d12::descriptor_page_allocator::initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, int num_descriptors, int page_size)
{
    _page_allocator.initialize(num_descriptors, page_size);
    _descriptor_size = device.GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = UINT(num_descriptors);
    desc.Type = type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    PR_D3D12_VERIFY(device.CreateDescriptorHeap(&desc, PR_COM_WRITE(_heap)));
    _heap->SetName(L"DynamicDescriptorRing::mHeap");

    _heap_start_cpu = _heap->GetCPUDescriptorHandleForHeapStart();
    _heap_start_gpu = _heap->GetGPUDescriptorHandleForHeapStart();
}

pr::backend::handle::shader_view pr::backend::d3d12::shader_view_allocator::create(ID3D12Device& device,
                                                                                   cc::span<pr::backend::handle::resource> srvs,
                                                                                   cc::span<pr::backend::handle::resource> uavs)
{
    auto const total_size = int(srvs.size() + uavs.size());
    descriptor_page_allocator::handle_t res_alloc;
    {
        std::lock_guard lg(_mutex);
        res_alloc = _srv_uav_allocator.allocate(total_size);
    }

    // Populate the data entry and fill out descriptors
    {
        auto& data = _shader_view_data[unsigned(res_alloc)];
        data.resources.clear();

        // Create the descriptors in-place
        {
            auto const cpu_base = _srv_uav_allocator.get_cpu_start(res_alloc);
            auto descriptor_index = 0u;

            for (auto const srv : srvs)
            {
                data.resources.push_back(srv);

                // TODO
                CC_RUNTIME_ASSERT(false && "Unimplemented");
                ID3D12Resource* const raw_resource = nullptr; // ???::get_raw_resource(srv);
                auto const cpu_handle = _srv_uav_allocator.increment_to_index(cpu_base, descriptor_index++);

                // Create a default SRV
                // (NOTE: Eventually we need more detailed views)
                device.CreateShaderResourceView(raw_resource, nullptr, cpu_handle);
            }

            for (auto const uav : uavs)
            {
                data.resources.push_back(uav);

                // TODO
                CC_RUNTIME_ASSERT(false && "Unimplemented");
                ID3D12Resource* const raw_resource = nullptr; // ???::get_raw_resource(uav);
                auto const gpu_handle = _srv_uav_allocator.increment_to_index(cpu_base, descriptor_index++);

                // Create a default UAV, without a counter resource
                // (NOTE: Eventually we need more detailed views)
                device.CreateUnorderedAccessView(raw_resource, nullptr, nullptr, gpu_handle);
            }
        }

        data.gpu_handle = _srv_uav_allocator.get_gpu_start(res_alloc);
        data.num_srvs = cc::uint16(srvs.size());
        data.num_uavs = cc::uint16(uavs.size());
    }

    return {res_alloc};
}
