#include "resource_creation.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>
#include <phantasm-renderer/backend/d3d12/memory/UploadHeap.hh>
#include <phantasm-renderer/backend/d3d12/memory/byte_util.hh>

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
    clear_value.Color[3] = 0.0f;

    auto const desc = CD3DX12_RESOURCE_DESC::Tex2D(format, UINT(w), UINT(h), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    auto res = allocator.allocateResource(desc, D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value);
    res.raw->SetName(L"anon render target");
    return res;
}

pr::backend::d3d12::resource pr::backend::d3d12::create_texture2d(pr::backend::d3d12::ResourceAllocator& allocator, int w, int h, DXGI_FORMAT format, int mips, D3D12_RESOURCE_FLAGS flags)
{
    auto const desc = CD3DX12_RESOURCE_DESC::Tex2D(format, UINT(w), UINT(h), 1, UINT16(mips), 1, 0, flags);

    auto res = allocator.allocateResource(desc, D3D12_RESOURCE_STATE_COMMON);
    res.raw->SetName(L"anon texture2d");
    return res;
}

pr::backend::d3d12::resource pr::backend::d3d12::create_buffer(pr::backend::d3d12::ResourceAllocator& allocator, size_t size_bytes)
{
    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(size_bytes);

    auto res = allocator.allocateResource(desc, D3D12_RESOURCE_STATE_COMMON);
    res.raw->SetName(L"anon buffer");
    return res;
}

void pr::backend::d3d12::make_rtv(const pr::backend::d3d12::resource& res, pr::backend::d3d12::resource_view& rtv, unsigned index, int mip)
{
    auto const texDesc = res.raw->GetDesc();

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
    res.raw->GetDevice(PR_COM_WRITE(device));
    device->CreateRenderTargetView(res.raw, &rtvDesc, rtv.get_cpu(index));
}

void pr::backend::d3d12::make_srv(const pr::backend::d3d12::resource& res, pr::backend::d3d12::resource_view& srv, unsigned index, int mip)
{
    auto const resource_desc = res.raw->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    if (resource_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        CC_RUNTIME_ASSERT(false && "unimplemented, no way to recover structured buffer stride here");
        srvDesc.Format = resource_desc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = UINT(resource_desc.Width);
        srvDesc.Buffer.StructureByteStride = 0 /*mStructuredBufferStride*/; // TODO
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    }
    else
    {
        if (resource_desc.Format == DXGI_FORMAT_R32_TYPELESS)
        {
            srvDesc.Format = DXGI_FORMAT_R32_FLOAT; // special case for the depth buffer
        }
        else
        {
            srvDesc.Format = resource_desc.Format;
        }

        if (resource_desc.SampleDesc.Count == 1)
        {
            if (resource_desc.DepthOrArraySize == 1)
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                if (mip == -1)
                {
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.MipLevels = resource_desc.MipLevels;
                }
                else
                {
                    srvDesc.Texture2D.MostDetailedMip = UINT(mip);
                    srvDesc.Texture2D.MipLevels = 1;
                }
            }
            else
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                if (mip == -1)
                {
                    srvDesc.Texture2DArray.MostDetailedMip = 0;
                    srvDesc.Texture2DArray.MipLevels = resource_desc.MipLevels;
                    srvDesc.Texture2DArray.ArraySize = resource_desc.DepthOrArraySize;
                }
                else
                {
                    srvDesc.Texture2DArray.MostDetailedMip = UINT(mip);
                    srvDesc.Texture2DArray.MipLevels = 1;
                    srvDesc.Texture2DArray.ArraySize = resource_desc.DepthOrArraySize;
                }
            }
        }
        else
        {
            if (resource_desc.DepthOrArraySize == 1)
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
            }
            else
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                if (mip == -1)
                {
                    srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    srvDesc.Texture2DMSArray.ArraySize = resource_desc.DepthOrArraySize;
                }
                else
                {
                    srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    srvDesc.Texture2DMSArray.ArraySize = resource_desc.DepthOrArraySize;
                }
            }
        }
    }

    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shared_com_ptr<ID3D12Device> device;
    res.raw->GetDevice(PR_COM_WRITE(device));
    device->CreateShaderResourceView(res.raw, &srvDesc, srv.get_cpu(index));
}

void pr::backend::d3d12::make_dsv(const pr::backend::d3d12::resource& res, pr::backend::d3d12::resource_view& dsv, unsigned index, int array_slice)
{
    auto const resource_desc = res.raw->GetDesc();

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
    res.raw->GetDevice(PR_COM_WRITE(device));
    device->CreateDepthStencilView(res.raw, &DSViewDesc, dsv.get_cpu(index));
}

void pr::backend::d3d12::make_uav(const pr::backend::d3d12::resource& res, pr::backend::d3d12::resource_view& uav, unsigned index)
{
    auto const resource_desc = res.raw->GetDesc();

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAViewDesc = {};
    UAViewDesc.Format = resource_desc.Format;
    UAViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    shared_com_ptr<ID3D12Device> device;
    res.raw->GetDevice(PR_COM_WRITE(device));
    device->CreateUnorderedAccessView(res.raw, nullptr, &UAViewDesc, uav.get_cpu(index));
}

void pr::backend::d3d12::make_cube_srv(const pr::backend::d3d12::resource& res, pr::backend::d3d12::resource_view& srv, unsigned index)
{
    auto const resource_desc = res.raw->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = resource_desc.Format;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.TextureCube.MostDetailedMip = 0;
    srv_desc.TextureCube.ResourceMinLODClamp = 0;
    srv_desc.TextureCube.MipLevels = resource_desc.MipLevels;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shared_com_ptr<ID3D12Device> device;
    res.raw->GetDevice(PR_COM_WRITE(device));
    device->CreateShaderResourceView(res.raw, &srv_desc, srv.get_cpu(index));
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

pr::backend::d3d12::resource pr::backend::d3d12::create_texture2d_from_file(
    pr::backend::d3d12::ResourceAllocator& allocator, ID3D12Device& device, pr::backend::d3d12::UploadHeap& upload_heap, const char* filename, bool use_srgb)
{
    img::image_info img_info;
    auto const img_handle = img::load_image(filename, img_info);
    CC_RUNTIME_ASSERT(img::is_valid(img_handle) && "File not found or image invalid");

    if (use_srgb)
        img_info.format = util::to_srgb_format(img_info.format);

    resource res = create_texture2d(allocator, img_info.width, img_info.height, img_info.format, img_info.mipMapCount);

    // Get mip footprints (if it is an array we reuse the mip footprints for all the elements of the array)
    //
    uint32_t num_rows[D3D12_REQ_MIP_LEVELS] = {0};
    UINT64 row_sizes_in_bytes[D3D12_REQ_MIP_LEVELS] = {0};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_subres_tex2d[D3D12_REQ_MIP_LEVELS];

    auto const res_desc = CD3DX12_RESOURCE_DESC::Tex2D(img_info.format, img_info.width, img_info.height, 1, img_info.mipMapCount);

    UINT64 upload_heap_size;
    device.GetCopyableFootprints(&res_desc, 0, img_info.mipMapCount, 0, placed_subres_tex2d, num_rows, row_sizes_in_bytes, &upload_heap_size);

    // compute pixel size
    //
    UINT32 bytes_per_pixel = img_info.bitCount / 8;
    if ((img_info.format >= DXGI_FORMAT_BC1_TYPELESS) && (img_info.format <= DXGI_FORMAT_BC5_SNORM))
    {
        bytes_per_pixel = util::get_dxgi_bytes_per_pixel(img_info.format);
    }

    for (auto a = 0u; a < img_info.arraySize; ++a)
    {
        // allocate memory for mip chain from upload heap
        //
        auto* const pixels = upload_heap.suballocateAllowRetry(SIZE_T(upload_heap_size), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

        // copy all the mip slices into the offsets specified by the footprint structure
        //
        for (auto mip = 0u; mip < img_info.mipMapCount; ++mip)
        {
            img::copy_pixels(img_handle, pixels + placed_subres_tex2d[mip].Offset, placed_subres_tex2d[mip].Footprint.RowPitch,
                             placed_subres_tex2d[mip].Footprint.Width * bytes_per_pixel, num_rows[mip]);

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT slice = placed_subres_tex2d[mip];
            slice.Offset += (pixels - upload_heap.getBasePointer());

            CD3DX12_TEXTURE_COPY_LOCATION Dst(res.raw, a * img_info.mipMapCount + mip);
            CD3DX12_TEXTURE_COPY_LOCATION Src(upload_heap.getResource(), slice);
            upload_heap.getCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
        }
    }

    // transition to (ps | non-ps) state
    D3D12_RESOURCE_BARRIER barrier_desc = {};
    barrier_desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_desc.Transition.pResource = res.raw;
    barrier_desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier_desc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier_desc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    upload_heap.getCommandList()->ResourceBarrier(1, &barrier_desc);

    return res;
}

pr::backend::d3d12::resource pr::backend::d3d12::create_buffer_from_data(pr::backend::d3d12::ResourceAllocator& allocator,
                                                                         pr::backend::d3d12::UploadHeap& upload_heap,
                                                                         size_t size,
                                                                         const void* data)
{
    auto res = create_buffer(allocator, size);
    auto const buffer_width = res.raw->GetDesc().Width;
    auto const buffer_align = res.raw->GetDesc().Alignment;

    auto* const memory = upload_heap.suballocateAllowRetry(buffer_width, buffer_align);
    ::memcpy(memory, data, size);

    upload_heap.copyAllocationToBuffer(res.raw, memory, buffer_width);

    return res;
}
