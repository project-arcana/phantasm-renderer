#include "shader_view_pool.hh"

#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>
#include <phantasm-renderer/backend/d3d12/common/native_enum.hh>
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

pr::backend::handle::shader_view pr::backend::d3d12::ShaderViewPool::create(cc::span<const arg::shader_view_element> srvs,
                                                                            cc::span<const arg::shader_view_element> uavs)
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
                data.resources.push_back(srv.resource);

                ID3D12Resource* const raw_resource = mResourcePool->getRawResource(srv.resource);
                auto const cpu_handle = mSRVUAVAllocator.incrementToIndex(cpu_base, descriptor_index++);

                // Create a SRV based on the shader_view_element
                D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
                srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srv_desc.Format = util::to_dxgi_format(srv.pixel_format);
                srv_desc.ViewDimension = util::to_native_srv_dim(srv.dimension);

                using svd = shader_view_dimension;
                switch (srv.dimension)
                {
                case svd::buffer:
                    srv_desc.Buffer.NumElements = srv.buffer_info.element_size;
                    srv_desc.Buffer.FirstElement = srv.buffer_info.element_start;
                    srv_desc.Buffer.StructureByteStride = srv.buffer_info.element_stride_bytes;
                    break;

                case svd::raytracing_accel_struct:
                    srv_desc.RaytracingAccelerationStructure.Location = raw_resource->GetGPUVirtualAddress();
                    break;

                case svd::texture1d:
                    srv_desc.Texture1D.MostDetailedMip = srv.texture_info.mip_start;
                    srv_desc.Texture1D.MipLevels = srv.texture_info.mip_size;
                    srv_desc.Texture1D.ResourceMinLODClamp = 0.f;
                    break;

                case svd::texture1d_array:
                    srv_desc.Texture1DArray.MostDetailedMip = srv.texture_info.mip_start;
                    srv_desc.Texture1DArray.MipLevels = srv.texture_info.mip_size;
                    srv_desc.Texture1DArray.ResourceMinLODClamp = 0.f;
                    srv_desc.Texture1DArray.FirstArraySlice = srv.texture_info.array_start;
                    srv_desc.Texture1DArray.ArraySize = srv.texture_info.array_size;
                    break;

                case svd::texture2d:
                    srv_desc.Texture2D.MostDetailedMip = srv.texture_info.mip_start;
                    srv_desc.Texture2D.MipLevels = srv.texture_info.mip_size;
                    srv_desc.Texture2D.ResourceMinLODClamp = 0.f;
                    srv_desc.Texture2D.PlaneSlice = 0u;
                    break;

                case svd::texture2d_array:
                    srv_desc.Texture2DArray.MostDetailedMip = srv.texture_info.mip_start;
                    srv_desc.Texture2DArray.MipLevels = srv.texture_info.mip_size;
                    srv_desc.Texture2DArray.ResourceMinLODClamp = 0.f;
                    srv_desc.Texture2DArray.PlaneSlice = 0u;
                    srv_desc.Texture2DArray.FirstArraySlice = srv.texture_info.array_start;
                    srv_desc.Texture2DArray.ArraySize = srv.texture_info.array_size;
                    break;

                case svd::texture2d_ms:
                    break;

                case svd::texture2d_ms_array:
                    srv_desc.Texture2DMSArray.FirstArraySlice = srv.texture_info.array_start;
                    srv_desc.Texture2DMSArray.ArraySize = srv.texture_info.array_size;
                    break;

                case svd::texture3d:
                    srv_desc.Texture3D.MostDetailedMip = srv.texture_info.mip_start;
                    srv_desc.Texture3D.MipLevels = srv.texture_info.mip_size;
                    srv_desc.Texture3D.ResourceMinLODClamp = 0.f;
                    break;

                case svd::texturecube:
                    srv_desc.TextureCube.MostDetailedMip = srv.texture_info.mip_start;
                    srv_desc.TextureCube.MipLevels = srv.texture_info.mip_size;
                    srv_desc.TextureCube.ResourceMinLODClamp = 0.f;
                    break;

                case svd::texturecube_array:
                    srv_desc.TextureCubeArray.MostDetailedMip = srv.texture_info.mip_start;
                    srv_desc.TextureCubeArray.MipLevels = srv.texture_info.mip_size;
                    srv_desc.TextureCubeArray.ResourceMinLODClamp = 0.f;
                    srv_desc.TextureCubeArray.First2DArrayFace = srv.texture_info.array_start;
                    srv_desc.TextureCubeArray.NumCubes = srv.texture_info.array_size;
                    break;
                }

                mDevice->CreateShaderResourceView(raw_resource, &srv_desc, cpu_handle);
            }

            for (auto const uav : uavs)
            {
                data.resources.push_back(uav.resource);

                ID3D12Resource* const raw_resource = mResourcePool->getRawResource(uav.resource);
                auto const cpu_handle = mSRVUAVAllocator.incrementToIndex(cpu_base, descriptor_index++);

                // Create a SRV based on the shader_view_element
                D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
                uav_desc.Format = util::to_dxgi_format(uav.pixel_format);
                uav_desc.ViewDimension = util::to_native_uav_dim(uav.dimension);
                CC_ASSERT(uav_desc.ViewDimension != D3D12_UAV_DIMENSION_UNKNOWN && "Invalid UAV dimension");

                using svd = shader_view_dimension;
                switch (uav.dimension)
                {
                case svd::buffer:
                    uav_desc.Buffer.NumElements = uav.buffer_info.element_size;
                    uav_desc.Buffer.FirstElement = uav.buffer_info.element_start;
                    uav_desc.Buffer.StructureByteStride = uav.buffer_info.element_stride_bytes;
                    break;

                case svd::texture1d:
                    uav_desc.Texture1D.MipSlice = uav.texture_info.mip_start;
                    break;

                case svd::texture1d_array:
                    uav_desc.Texture1DArray.MipSlice = uav.texture_info.mip_start;
                    uav_desc.Texture1DArray.FirstArraySlice = uav.texture_info.array_start;
                    uav_desc.Texture1DArray.ArraySize = uav.texture_info.array_size;
                    break;

                case svd::texture2d:
                    uav_desc.Texture2D.MipSlice = uav.texture_info.mip_start;
                    uav_desc.Texture2D.PlaneSlice = 0u;
                    break;

                case svd::texture2d_array:
                    uav_desc.Texture2DArray.MipSlice = uav.texture_info.mip_start;
                    uav_desc.Texture2DArray.PlaneSlice = 0u;
                    uav_desc.Texture2DArray.FirstArraySlice = uav.texture_info.array_start;
                    uav_desc.Texture2DArray.ArraySize = uav.texture_info.array_size;
                    break;

                case svd::texture3d:
                    uav_desc.Texture3D.FirstWSlice = uav.texture_info.array_start;
                    uav_desc.Texture3D.WSize = uav.texture_info.array_size;
                    uav_desc.Texture3D.MipSlice = uav.texture_info.mip_start;
                    break;
                }

                // Create a  without a counter resource
                mDevice->CreateUnorderedAccessView(raw_resource, nullptr, &uav_desc, cpu_handle);
            }
        }

        data.gpu_handle = mSRVUAVAllocator.getGPUStart(res_alloc);
        data.num_srvs = cc::uint16(srvs.size());
        data.num_uavs = cc::uint16(uavs.size());
    }

    return {res_alloc};
}

void pr::backend::d3d12::ShaderViewPool::initialize(ID3D12Device* device, pr::backend::d3d12::ResourcePool* res_pool, int num_srvs_uavs)
{
    mDevice = device;
    mResourcePool = res_pool;
    mSRVUAVAllocator.initialize(*device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, num_srvs_uavs);
    mShaderViewData = cc::array<shader_view_data>::uninitialized(unsigned(mSRVUAVAllocator.getNumPages()));
}
