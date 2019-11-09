#include "Texture.hh"
// AMD AMDUtils code
//
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "dxgi.h"

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>

namespace pr::backend::d3d12
{
void Texture::patchFmt24To32Bit(unsigned char* pDst, unsigned char* pSrc, UINT32 pixelCount)
{
    // copy pixel data, interleave with A
    for (unsigned int i = 0; i < pixelCount; ++i)
    {
        pDst[0] = pSrc[0];
        pDst[1] = pSrc[1];
        pDst[2] = pSrc[2];
        pDst[3] = 0xFF;
        pDst += 4;
        pSrc += 3;
    }
}

bool Texture::isCubemap() const { return mHeader.arraySize == 6; }

INT32 Texture::initRenderTarget(ID3D12Device& device, const char* /*debug_name*/, CD3DX12_RESOURCE_DESC const& desc, D3D12_RESOURCE_STATES state)
{
    // Performance tip: Tell the runtime at resource creation the desired clear value.
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = desc.Format;
    clearValue.Color[0] = 0.0f;
    clearValue.Color[1] = 0.0f;
    clearValue.Color[2] = 0.0f;
    clearValue.Color[3] = 0.0f;

    bool isRenderTarget = desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ? 1 : 0;

    auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device.CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &desc, state, isRenderTarget ? &clearValue : nullptr, PR_COM_WRITE(mResource));

    mHeader.width = UINT32(desc.Width);
    mHeader.height = desc.Height;
    mHeader.mipMapCount = desc.MipLevels;
    mHeader.format = desc.Format;

    //    m_pResource->SetName(pDebugName); TODO
    //    SetName(m_pResource, pDebugName);

    return 0;
}

bool Texture::initBuffer(ID3D12Device& device, const char* /*debug_name*/, CD3DX12_RESOURCE_DESC const& desc, uint32_t structureSize, D3D12_RESOURCE_STATES state)
{
    CC_ASSERT(desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && desc.Height == 1 && desc.MipLevels == 1);

    D3D12_RESOURCE_DESC desc_copy = desc;

    if (desc_copy.Format != DXGI_FORMAT_UNKNOWN)
    {
        // Formatted buffer
        CC_ASSERT(structureSize == 0);
        mStructuredBufferStride = 0;
        mHeader.width = UINT32(desc_copy.Width);
        mHeader.format = desc_copy.Format;

        // Fix up the D3D12_RESOURCE_DESC to be a typeless buffer.  The type/format will be associated with the UAV/SRV
        desc_copy.Format = DXGI_FORMAT_UNKNOWN;
        desc_copy.Width = util::get_dxgi_bytes_per_pixel(mHeader.format) * desc_copy.Width;
    }
    else
    {
        // Structured buffer
        mStructuredBufferStride = structureSize;
        mHeader.width = UINT32(desc_copy.Width) / mStructuredBufferStride;
        mHeader.format = DXGI_FORMAT_UNKNOWN;
    }

    mHeader.height = 1;
    mHeader.mipMapCount = 1;

    auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto hres = device.CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &desc_copy, state, nullptr, PR_COM_WRITE(mResource));

    // SetName(mResource, debug_name);

    return hres == S_OK;
}

bool Texture::initCounter(ID3D12Device& device, const char* debug_name, CD3DX12_RESOURCE_DESC const& desc, uint32_t counterSize, D3D12_RESOURCE_STATES state)
{
    return initBuffer(device, debug_name, desc, counterSize, state);
}

void Texture::createRTV(uint32_t index, RViewRTV& pRV, int mipLevel)
{
    D3D12_RESOURCE_DESC texDesc = mResource->GetDesc();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texDesc.Format;
    if (texDesc.SampleDesc.Count == 1)
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    else
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

    if (mipLevel == -1)
    {
        rtvDesc.Texture2D.MipSlice = 0;
    }
    else
    {
        rtvDesc.Texture2D.MipSlice = mipLevel;
    }

    shared_com_ptr<ID3D12Device> device;
    mResource->GetDevice(PR_COM_WRITE(device));
    device->CreateRenderTargetView(mResource, &rtvDesc, pRV.getCPU(index));
}


bool Texture::initFromData(ID3D12Device& device, const char* debug_name, UploadHeap& uploadHeap, const img::image_info& header, const void* data)
{
    CC_ASSERT(!mResource);
    CC_ASSERT(header.arraySize == 1 && header.mipMapCount == 1);

    mHeader = header;

    createTextureCommitted(device, debug_name, false);

    // Get mip footprints (if it is an array we reuse the mip footprints for all the elements of the array)
    UINT64 UplHeapSize = 0;
    uint32_t num_rows = 0;
    UINT64 row_sizes_in_bytes = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTex2D = {};
    CD3DX12_RESOURCE_DESC RDescs = CD3DX12_RESOURCE_DESC::Tex2D((DXGI_FORMAT)mHeader.format, mHeader.width, mHeader.height, 1, 1);
    device.GetCopyableFootprints(&RDescs, 0, 1, 0, &placedTex2D, &num_rows, &row_sizes_in_bytes, &UplHeapSize);


    // allocate memory for mip chain from upload heap
    UINT8* pixels = uploadHeap.suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    if (pixels == nullptr)
    {
        // oh! We ran out of mem in the upload heap, flush it and try allocating mem from it again
        uploadHeap.flushAndFinish();
        pixels = uploadHeap.suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        CC_ASSERT(pixels);
    }

    placedTex2D.Offset += UINT64(pixels - uploadHeap.getBasePointer());

    // copy all the mip slices into the offsets specified by the footprint structure
    //
    for (auto y = 0u; y < num_rows; ++y)
    {
        memcpy(pixels + y * placedTex2D.Footprint.RowPitch, static_cast<UINT8 const*>(data) + y * placedTex2D.Footprint.RowPitch, row_sizes_in_bytes);
    }

    CD3DX12_TEXTURE_COPY_LOCATION Dst(mResource, 0);
    CD3DX12_TEXTURE_COPY_LOCATION Src(uploadHeap.getResource(), placedTex2D);
    uploadHeap.getCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);


    // prepare to shader read
    //
    D3D12_RESOURCE_BARRIER RBDesc = {};
    RBDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    RBDesc.Transition.pResource = mResource;
    RBDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    RBDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    RBDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    uploadHeap.getCommandList()->ResourceBarrier(1, &RBDesc);

    return true;
}

void Texture::createUAV(uint32_t index, RViewCBV_SRV_UAV& pRV)
{
    auto const resource_desc = mResource->GetDesc();

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAViewDesc = {};
    UAViewDesc.Format = resource_desc.Format;
    UAViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    shared_com_ptr<ID3D12Device> device;
    mResource->GetDevice(PR_COM_WRITE(device));
    device->CreateUnorderedAccessView(mResource, nullptr, &UAViewDesc, pRV.getCPU(index));
}

void Texture::createBufferUAV(uint32_t index, Texture* counter_tex, RViewCBV_SRV_UAV& pRV)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = mHeader.format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = mHeader.width;
    uavDesc.Buffer.StructureByteStride = mStructuredBufferStride;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    shared_com_ptr<ID3D12Device> device;
    mResource->GetDevice(PR_COM_WRITE(device));
    device->CreateUnorderedAccessView(mResource, counter_tex ? counter_tex->getResource() : nullptr, &uavDesc, pRV.getCPU(index));
}

void Texture::createSRV(uint32_t index, RViewCBV_SRV_UAV& pRV, int mipLevel)
{
    auto const resource_desc = mResource->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    if (resource_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        srvDesc.Format = resource_desc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srvDesc.Buffer.FirstElement = 0;
        srvDesc.Buffer.NumElements = mHeader.width;
        srvDesc.Buffer.StructureByteStride = mStructuredBufferStride;
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
                if (mipLevel == -1)
                {
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    srvDesc.Texture2D.MipLevels = mHeader.mipMapCount;
                }
                else
                {
                    srvDesc.Texture2D.MostDetailedMip = mipLevel;
                    srvDesc.Texture2D.MipLevels = 1;
                }
            }
            else
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                if (mipLevel == -1)
                {
                    srvDesc.Texture2DArray.MostDetailedMip = 0;
                    srvDesc.Texture2DArray.MipLevels = mHeader.mipMapCount;
                    srvDesc.Texture2DArray.ArraySize = resource_desc.DepthOrArraySize;
                }
                else
                {
                    srvDesc.Texture2DArray.MostDetailedMip = mipLevel;
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
                if (mipLevel == -1)
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
    mResource->GetDevice(PR_COM_WRITE(device));
    device->CreateShaderResourceView(mResource, &srvDesc, pRV.getCPU(index));
}

void Texture::createCubeSRV(uint32_t index, RViewCBV_SRV_UAV& out_rv)
{
    auto const resource_desc = mResource->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = resource_desc.Format;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srv_desc.TextureCube.MostDetailedMip = 0;
    srv_desc.TextureCube.ResourceMinLODClamp = 0;
    srv_desc.TextureCube.MipLevels = resource_desc.MipLevels;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    shared_com_ptr<ID3D12Device> device;
    mResource->GetDevice(PR_COM_WRITE(device));
    device->CreateShaderResourceView(mResource, &srv_desc, out_rv.getCPU(index));
}

void Texture::createDSV(uint32_t index, RViewDSV& out_rv, int array_slice)
{
    auto const resource_desc = mResource->GetDesc();

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
    mResource->GetDevice(PR_COM_WRITE(device));
    device->CreateDepthStencilView(mResource, &DSViewDesc, out_rv.getCPU(index));
}

INT32 Texture::initDepthStencil(ID3D12Device& device, const char* /*debug_name*/, CD3DX12_RESOURCE_DESC const& desc)
{
    // Performance tip: Tell the runtime at resource creation the desired clear value.
    D3D12_CLEAR_VALUE clear_value;
    clear_value.Format = desc.Format;
    clear_value.DepthStencil.Depth = 1.0f;
    clear_value.DepthStencil.Stencil = 0;

    D3D12_RESOURCE_STATES states = D3D12_RESOURCE_STATE_DEPTH_WRITE;
    // if (desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)
    //    states |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    device.CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &desc, states, &clear_value, PR_COM_WRITE(mResource));

    mHeader.width = UINT32(desc.Width);
    mHeader.height = desc.Height;
    mHeader.depth = desc.Depth();
    mHeader.mipMapCount = desc.MipLevels;
    mHeader.format = desc.Format;

    //    SetName(m_pResource, pDebugName);

    return 0;
}

//--------------------------------------------------------------------------------------
// create a comitted resource using m_header
//--------------------------------------------------------------------------------------
void Texture::createTextureCommitted(ID3D12Device& device, const char* /*debug_name*/, bool useSRGB)
{
    if (useSRGB)
    {
        mHeader.format = util::to_srgb_format(mHeader.format);
    }

    auto const res_desc = CD3DX12_RESOURCE_DESC::Tex2D(mHeader.format, mHeader.width, mHeader.height, mHeader.arraySize, mHeader.mipMapCount);
    auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    device.CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &res_desc, D3D12_RESOURCE_STATE_COMMON, nullptr, PR_COM_WRITE(mResource));

    //    SetName(m_pResource, pDebugName);
}

void Texture::loadAndUpload(ID3D12Device& device, UploadHeap& upload_heap, img::image_handle const& handle)
{
    // Get mip footprints (if it is an array we reuse the mip footprints for all the elements of the array)
    //
    uint32_t num_rows[D3D12_REQ_MIP_LEVELS] = {0};
    UINT64 row_sizes_in_bytes[D3D12_REQ_MIP_LEVELS] = {0};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_subres_tex2d[D3D12_REQ_MIP_LEVELS];

    auto const res_desc = CD3DX12_RESOURCE_DESC::Tex2D(mHeader.format, mHeader.width, mHeader.height, 1, mHeader.mipMapCount);

    UINT64 UplHeapSize;
    device.GetCopyableFootprints(&res_desc, 0, mHeader.mipMapCount, 0, placed_subres_tex2d, num_rows, row_sizes_in_bytes, &UplHeapSize);

    // compute pixel size
    //
    UINT32 bytePP = mHeader.bitCount / 8;
    if ((mHeader.format >= DXGI_FORMAT_BC1_TYPELESS) && (mHeader.format <= DXGI_FORMAT_BC5_SNORM))
    {
        bytePP = util::get_dxgi_bytes_per_pixel(mHeader.format);
    }

    for (auto a = 0u; a < mHeader.arraySize; ++a)
    {
        // allocate memory for mip chain from upload heap
        //
        auto* pixels = upload_heap.suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        if (pixels == nullptr)
        {
            // oh! We ran out of mem in the upload heap, flush it and try allocating mem from it again
            upload_heap.flushAndFinish();
            pixels = upload_heap.suballocate(SIZE_T(UplHeapSize), D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
            CC_ASSERT(pixels != nullptr);
        }

        // copy all the mip slices into the offsets specified by the footprint structure
        //
        for (auto mip = 0u; mip < mHeader.mipMapCount; ++mip)
        {
            img::copy_pixels(handle, pixels + placed_subres_tex2d[mip].Offset, placed_subres_tex2d[mip].Footprint.RowPitch,
                             placed_subres_tex2d[mip].Footprint.Width * bytePP, num_rows[mip]);

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT slice = placed_subres_tex2d[mip];
            slice.Offset += (pixels - upload_heap.getBasePointer());

            CD3DX12_TEXTURE_COPY_LOCATION Dst(mResource, a * mHeader.mipMapCount + mip);
            CD3DX12_TEXTURE_COPY_LOCATION Src(upload_heap.getResource(), slice);
            upload_heap.getCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
        }
    }

    // transition to (ps | non-ps) state
    D3D12_RESOURCE_BARRIER barrier_desc = {};
    barrier_desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_desc.Transition.pResource = mResource;
    barrier_desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier_desc.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier_desc.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    upload_heap.getCommandList()->ResourceBarrier(1, &barrier_desc);
}

//--------------------------------------------------------------------------------------
// entry function to initialize an image from a .DDS texture
//--------------------------------------------------------------------------------------
bool Texture::initFromFile(ID3D12Device& device, UploadHeap& upload_heap, const char* filename, bool use_srgb)
{
    CC_ASSERT(mResource == nullptr);

    auto const res_handle = img::load_image(filename, mHeader);
    if (img::is_valid(res_handle))
    {
        createTextureCommitted(device, filename, use_srgb);
        loadAndUpload(device, upload_heap, res_handle);
        img::free(res_handle);
        return true;
    }
    else
    {
        return false;
    }
}
}
