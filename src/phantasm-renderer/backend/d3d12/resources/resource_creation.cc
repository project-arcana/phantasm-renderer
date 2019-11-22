#include "resource_creation.hh"

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <typed-geometry/tg.hh>

#include <phantasm-renderer/backend/assets/image_loader.hh>

#include <phantasm-renderer/backend/command_stream.hh>
#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>
#include <phantasm-renderer/backend/d3d12/memory/UploadHeap.hh>
#include <phantasm-renderer/backend/detail/byte_util.hh>

#include "image_load_util.hh"

pr::backend::d3d12::resource pr::backend::d3d12::create_depth_stencil(pr::backend::d3d12::ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format)
{
    D3D12_CLEAR_VALUE clear_value;
    clear_value.Format = format;
    clear_value.DepthStencil.Depth = 1;
    clear_value.DepthStencil.Stencil = 0;

    auto const desc = CD3DX12_RESOURCE_DESC::Tex2D(format, UINT(w), UINT(h), 1, 0, 1, 0,
                                                   D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

    auto res = allocator.allocateResource(desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value);
    res.raw->SetName(L"anon depth stencil");
    return res;
}

pr::backend::d3d12::resource pr::backend::d3d12::create_render_target(pr::backend::d3d12::ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format)
{
    D3D12_CLEAR_VALUE clear_value;
    clear_value.Format = format;
    clear_value.Color[0] = 0.0f;
    clear_value.Color[1] = 0.0f;
    clear_value.Color[2] = 0.0f;
    clear_value.Color[3] = 1.0f;

    auto const desc = CD3DX12_RESOURCE_DESC::Tex2D(format, UINT(w), UINT(h), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    auto res = allocator.allocateResource(desc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value);
    res.raw->SetName(L"anon render target");
    return res;
}

pr::backend::d3d12::resource pr::backend::d3d12::create_texture2d(pr::backend::d3d12::ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format, int mips, D3D12_RESOURCE_FLAGS flags)
{
    auto const desc = CD3DX12_RESOURCE_DESC::Tex2D(format, UINT(w), UINT(h), 1, UINT16(mips), 1, 0, flags);

    auto res = allocator.allocateResource(desc, D3D12_RESOURCE_STATE_COPY_DEST);
    res.raw->SetName(L"anon texture2d");
    return res;
}

pr::backend::d3d12::resource pr::backend::d3d12::create_buffer(pr::backend::d3d12::ResourceAllocator& allocator, size_t size_bytes, D3D12_RESOURCE_STATES initial_state)
{
    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(size_bytes);

    auto res = allocator.allocateResource(desc, initial_state);
    res.raw->SetName(L"anon buffer");
    return res;
}

void pr::backend::d3d12::make_rtv(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int mip)
{
    auto const texDesc = res->GetDesc();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texDesc.Format;
    if (texDesc.SampleDesc.Count == 1)
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    else
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

    if (mip == -1)
    {
        rtvDesc.Texture2D.MipSlice = 0;
    }
    else
    {
        rtvDesc.Texture2D.MipSlice = UINT(mip);
    }

    shared_com_ptr<ID3D12Device> device;
    res->GetDevice(PR_COM_WRITE(device));
    device->CreateRenderTargetView(res, &rtvDesc, handle);
}

void pr::backend::d3d12::make_srv(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int mip)
{
    auto const resource_desc = res->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};

    if (resource_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        CC_RUNTIME_ASSERT(false && "unimplemented, no way to recover structured buffer stride here");
        srv_desc.Format = resource_desc.Format;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = UINT(resource_desc.Width);
        srv_desc.Buffer.StructureByteStride = 0 /*mStructuredBufferStride*/; // TODO
        srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    }
    else
    {
        if (resource_desc.Format == DXGI_FORMAT_R32_TYPELESS)
        {
            srv_desc.Format = DXGI_FORMAT_R32_FLOAT; // special case for the depth buffer
        }
        else
        {
            srv_desc.Format = resource_desc.Format;
        }

        if (resource_desc.SampleDesc.Count == 1)
        {
            if (resource_desc.DepthOrArraySize == 1)
            {
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                if (mip == -1)
                {
                    srv_desc.Texture2D.MostDetailedMip = 0;
                    srv_desc.Texture2D.MipLevels = resource_desc.MipLevels;
                }
                else
                {
                    srv_desc.Texture2D.MostDetailedMip = UINT(mip);
                    srv_desc.Texture2D.MipLevels = 1;
                }
            }
            else
            {
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                if (mip == -1)
                {
                    srv_desc.Texture2DArray.MostDetailedMip = 0;
                    srv_desc.Texture2DArray.MipLevels = resource_desc.MipLevels;
                    srv_desc.Texture2DArray.ArraySize = resource_desc.DepthOrArraySize;
                }
                else
                {
                    srv_desc.Texture2DArray.MostDetailedMip = UINT(mip);
                    srv_desc.Texture2DArray.MipLevels = 1;
                    srv_desc.Texture2DArray.ArraySize = resource_desc.DepthOrArraySize;
                }
            }
        }
        else
        {
            if (resource_desc.DepthOrArraySize == 1)
            {
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                if (mip == -1)
                {
                    srv_desc.Texture2DMSArray.FirstArraySlice = 0;
                    srv_desc.Texture2DMSArray.ArraySize = resource_desc.DepthOrArraySize;
                }
                else
                {
                    srv_desc.Texture2DMSArray.FirstArraySlice = 0;
                    srv_desc.Texture2DMSArray.ArraySize = resource_desc.DepthOrArraySize;
                }
            }
        }
    }

    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shared_com_ptr<ID3D12Device> device;
    res->GetDevice(PR_COM_WRITE(device));
    device->CreateShaderResourceView(res, &srv_desc, handle);
}

void pr::backend::d3d12::make_dsv(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle, int array_slice)
{
    auto const resource_desc = res->GetDesc();

    D3D12_DEPTH_STENCIL_VIEW_DESC DSViewDesc = {};
    DSViewDesc.Format = resource_desc.Format;
    if (resource_desc.SampleDesc.Count == 1)
    {
        if (resource_desc.DepthOrArraySize == 1)
        {
            DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            DSViewDesc.Texture2D.MipSlice = 0;
        }
        else
        {
            DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            DSViewDesc.Texture2DArray.MipSlice = 0;
            DSViewDesc.Texture2DArray.FirstArraySlice = UINT(array_slice);
            DSViewDesc.Texture2DArray.ArraySize = 1; // resource_desc.DepthOrArraySize;
        }
    }
    else
    {
        DSViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    }

    shared_com_ptr<ID3D12Device> device;
    res->GetDevice(PR_COM_WRITE(device));
    device->CreateDepthStencilView(res, &DSViewDesc, handle);
}

void pr::backend::d3d12::make_uav(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    auto const resource_desc = res->GetDesc();

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAViewDesc = {};
    UAViewDesc.Format = resource_desc.Format;
    UAViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

    shared_com_ptr<ID3D12Device> device;
    res->GetDevice(PR_COM_WRITE(device));
    device->CreateUnorderedAccessView(res, nullptr, &UAViewDesc, handle);
}

void pr::backend::d3d12::make_cube_srv(ID3D12Resource* res, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    auto const resource_desc = res->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = resource_desc.Format;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.TextureCube.MostDetailedMip = 0;
    srv_desc.TextureCube.ResourceMinLODClamp = 0;
    srv_desc.TextureCube.MipLevels = resource_desc.MipLevels;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shared_com_ptr<ID3D12Device> device;
    res->GetDevice(PR_COM_WRITE(device));
    device->CreateShaderResourceView(res, &srv_desc, handle);
}

D3D12_VERTEX_BUFFER_VIEW pr::backend::d3d12::make_vertex_buffer_view(const pr::backend::d3d12::resource& res, size_t vertex_size)
{
    return D3D12_VERTEX_BUFFER_VIEW{res.raw->GetGPUVirtualAddress(), UINT(res.raw->GetDesc().Width), UINT(vertex_size)};
}

D3D12_INDEX_BUFFER_VIEW pr::backend::d3d12::make_index_buffer_view(const pr::backend::d3d12::resource& res, size_t index_size)
{
    return D3D12_INDEX_BUFFER_VIEW{res.raw->GetGPUVirtualAddress(), UINT(res.raw->GetDesc().Width), (index_size == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT};
}

D3D12_CONSTANT_BUFFER_VIEW_DESC pr::backend::d3d12::make_constant_buffer_view(const pr::backend::d3d12::resource& res)
{
    return D3D12_CONSTANT_BUFFER_VIEW_DESC{res.raw->GetGPUVirtualAddress(), UINT(res.raw->GetDesc().Width)};
}

namespace
{
using namespace pr::backend::d3d12;
pr::backend::d3d12::resource create_texture2d_from_data(ResourceAllocator& allocator,
                                                        ID3D12Device& device,
                                                        UploadHeap& upload_heap,
                                                        DXGI_FORMAT format,
                                                        pr::backend::assets::image_size const& img_size,
                                                        pr::backend::assets::image_data const& img_data)
{
    resource res = create_texture2d(allocator, int(img_size.width), int(img_size.height), format, int(img_size.num_mipmaps));

    // Get mip footprints (if it is an array we reuse the mip footprints for all the elements of the array)
    //
    uint32_t num_rows[D3D12_REQ_MIP_LEVELS] = {0};
    UINT64 row_sizes_in_bytes[D3D12_REQ_MIP_LEVELS] = {0};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_subres_tex2d[D3D12_REQ_MIP_LEVELS];
    UINT64 upload_heap_size;
    {
        auto const res_desc = CD3DX12_RESOURCE_DESC::Tex2D(format, img_size.width, img_size.height, 1, UINT16(img_size.num_mipmaps));
        device.GetCopyableFootprints(&res_desc, 0, img_size.num_mipmaps, 0, placed_subres_tex2d, num_rows, row_sizes_in_bytes, &upload_heap_size);
    }

    auto const bytes_per_pixel = util::get_dxgi_bytes_per_pixel(format);

    for (auto a = 0u; a < img_size.array_size; ++a)
    {
        // allocate memory for mip chain from upload heap
        //
        auto* const upload_ptr = upload_heap.suballocateAllowRetry(upload_heap_size, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        // copy all the mip slices into the offsets specified by the footprint structure
        //
        for (auto mip = 0u; mip < img_size.num_mipmaps; ++mip)
        {
            // D3D12_PLACED_SUBRESOURCE_FOOTPRINT slice = placed_subres_tex2d[mip];
            // slice.Offset += size_t(upload_ptr - upload_heap.getBasePointer());

            D3D12_SUBRESOURCE_FOOTPRINT ng_footprint;
            ng_footprint.Format = format;
            ng_footprint.Width = placed_subres_tex2d[mip].Footprint.Width;
            ng_footprint.Height = placed_subres_tex2d[mip].Footprint.Height;
            ng_footprint.Depth = 1;
            ng_footprint.RowPitch = placed_subres_tex2d[mip].Footprint.RowPitch;

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT ng_placed_footprint;
            ng_placed_footprint.Offset = placed_subres_tex2d[mip].Offset + size_t(upload_ptr - upload_heap.getBasePointer());
            ng_placed_footprint.Footprint = ng_footprint;

            pr::backend::assets::copy_subdata(img_data, upload_ptr + ng_placed_footprint.Offset, ng_footprint.RowPitch,
                                              ng_footprint.Width * bytes_per_pixel, ng_footprint.Height);

            CD3DX12_TEXTURE_COPY_LOCATION const source(upload_heap.getResource(), ng_placed_footprint);
            CD3DX12_TEXTURE_COPY_LOCATION const dest(res.raw, a * img_size.num_mipmaps + mip);
            upload_heap.getCommandList()->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
        }
    }

    return res;
}
}

pr::backend::d3d12::resource pr::backend::d3d12::create_texture2d_from_file(
    pr::backend::d3d12::ResourceAllocator& allocator, ID3D12Device& device, pr::backend::d3d12::UploadHeap& upload_heap, const char* filename, bool use_srgb)
{
#if 1
    {
        // new assets-based loading
        assets::image_size img_size;
        auto const img_data = assets::load_image(filename, img_size);
        CC_RUNTIME_ASSERT(assets::is_valid(img_data) && "File not found or image invalid");
        auto res = create_texture2d_from_data(allocator, device, upload_heap, use_srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM,
                                              img_size, img_data);
        assets::free(img_data);
        return res;
    }
#else
    {
        img::image_info img_info;
        auto const img_handle = img::load_image(filename, img_info);
        CC_RUNTIME_ASSERT(img::is_valid(img_handle) && "File not found or image invalid");

        if (use_srgb)
            img_info.format = util::to_srgb_format(img_info.format);

        auto res = create_texture2d_from_data(allocator, device, upload_heap, img_info, img_handle);
        img::free(img_handle);
        return res;
    }
#endif
}

pr::backend::d3d12::resource pr::backend::d3d12::create_buffer_from_data(pr::backend::d3d12::ResourceAllocator& allocator,
                                                                         pr::backend::d3d12::UploadHeap& upload_heap,
                                                                         size_t size,
                                                                         const void* data)
{
    auto res = create_buffer(allocator, size, D3D12_RESOURCE_STATE_COPY_DEST);
    auto const buffer_width = res.raw->GetDesc().Width;
    auto const buffer_align = res.raw->GetDesc().Alignment;

    auto* const memory = upload_heap.suballocateAllowRetry(buffer_width, buffer_align);
    ::memcpy(memory, data, size);

    upload_heap.copyAllocationToBuffer(res.raw, memory, buffer_width);

    return res;
}
