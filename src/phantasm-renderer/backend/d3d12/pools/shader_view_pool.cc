#include "shader_view_pool.hh"

#include <phantasm-renderer/backend/d3d12/common/util.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

#include "resource_pool.hh"

void pr::backend::d3d12::DescriptorPageAllocator::initialize(ID3D12Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, int num_descriptors, int page_size)
{
    mPageAllocator.initialize(num_descriptors, page_size);
    mDescriptorSize = device.GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.NumDescriptors = UINT(num_descriptors);
    desc.Type = type;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    PR_D3D12_VERIFY(device.CreateDescriptorHeap(&desc, PR_COM_WRITE(mHeap)));
    util::set_object_name(mHeap, "DescriptorPageAllocator t%d, s%d", int(type), int(num_descriptors));

    mHeapStartCPU = mHeap->GetCPUDescriptorHandleForHeapStart();
    mHeapStartGPU = mHeap->GetGPUDescriptorHandleForHeapStart();
}

pr::backend::handle::shader_view pr::backend::d3d12::ShaderViewPool::create(cc::span<pr::backend::handle::resource> srvs, cc::span<pr::backend::handle::resource> uavs)
{
    auto const total_size = int(srvs.size() + uavs.size());
    DescriptorPageAllocator::handle_t res_alloc;
    {
        auto lg = std::lock_guard(mMutex);
        res_alloc = mSRVUAVAllocator.allocate(total_size);
    }

    // Populate the data entry and fill out descriptors
    {
        auto& data = mShaderViewData[unsigned(res_alloc)];
        data.resources.clear();

        // Create the descriptors in-place
        {
            auto const cpu_base = mSRVUAVAllocator.getCPUStart(res_alloc);
            auto descriptor_index = 0u;

            for (auto const srv : srvs)
            {
                data.resources.push_back(srv);

                ID3D12Resource* const raw_resource = mResourcePool->getRawResource(srv);
                auto const cpu_handle = mSRVUAVAllocator.incrementToIndex(cpu_base, descriptor_index++);

                // Create a default SRV
                // (NOTE: Eventually we need more detailed views)
                mDevice->CreateShaderResourceView(raw_resource, nullptr, cpu_handle);
            }

            for (auto const uav : uavs)
            {
                data.resources.push_back(uav);

                ID3D12Resource* const raw_resource = mResourcePool->getRawResource(uav);
                auto const cpu_handle = mSRVUAVAllocator.incrementToIndex(cpu_base, descriptor_index++);

                // Create a default UAV, without a counter resource
                // (NOTE: Eventually we need more detailed views)
                mDevice->CreateUnorderedAccessView(raw_resource, nullptr, nullptr, cpu_handle);
            }
        }

        data.gpu_handle = mSRVUAVAllocator.getGPUStart(res_alloc);
        data.num_srvs = cc::uint16(srvs.size());
        data.num_uavs = cc::uint16(uavs.size());
    }

    return {res_alloc};
}

void pr::backend::d3d12::ShaderViewPool::initialize(ID3D12Device *device, pr::backend::d3d12::ResourcePool *res_pool, int num_srvs_uavs)
{
    mDevice = device;
    mResourcePool = res_pool;
    mSRVUAVAllocator.initialize(*device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num_srvs_uavs);
    mShaderViewData = cc::array<shader_view_data>::uninitialized(unsigned(mSRVUAVAllocator.getNumPages()));
}
