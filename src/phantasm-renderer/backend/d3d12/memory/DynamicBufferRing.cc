#include "DynamicBufferRing.hh"

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

#include "byte_util.hh"

namespace pr::backend::d3d12
{
//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void DynamicBufferRing::initialize(ID3D12Device& pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize)
{
    mTotalSize = mem::align_offset(memTotalSize, 256u);

    mMemoryRing.initialize(numberOfBackBuffers, memTotalSize);

    auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto const resource_desc = CD3DX12_RESOURCE_DESC::Buffer(memTotalSize);
    PR_D3D12_VERIFY(pDevice.CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                    PR_COM_WRITE(mResource)));
    mResource->SetName(L"DynamicBufferRing::m_pBuffer");

    mResource->Map(0, nullptr, reinterpret_cast<void**>(&mData));
}

//--------------------------------------------------------------------------------------
//
// AllocConstantBuffer
//
//--------------------------------------------------------------------------------------
bool DynamicBufferRing::allocConstantBuffer(uint32_t size, void*& out_data, D3D12_GPU_VIRTUAL_ADDRESS& out_buffer_view_desc)
{
    size = mem::align_offset(size, 256);

    uint32_t memOffset;
    if (mMemoryRing.alloc(size, &memOffset) == false)
    {
        // Trace("Ran out of mem for 'dynamic' buffers, please increase the allocated size\n");
        return false;
    }

    out_data = static_cast<void*>(mData + memOffset);

    out_buffer_view_desc = mResource->GetGPUVirtualAddress() + memOffset;

    return true;
}

//--------------------------------------------------------------------------------------
//
// AllocVertexBuffer
//
//--------------------------------------------------------------------------------------
bool DynamicBufferRing::allocVertexBuffer(uint32_t num_vertices, uint32_t stride_in_bytes, void*& out_data, D3D12_VERTEX_BUFFER_VIEW& out_view)
{
    auto const size = mem::align_offset(num_vertices * stride_in_bytes, 256);

    uint32_t memOffset;
    if (mMemoryRing.alloc(size, &memOffset) == false)
        return false;

    out_data = static_cast<void*>(mData + memOffset);

    out_view.BufferLocation = mResource->GetGPUVirtualAddress() + memOffset;
    out_view.StrideInBytes = stride_in_bytes;
    out_view.SizeInBytes = size;

    return true;
}

bool DynamicBufferRing::allocIndexBuffer(uint32_t num_indices, uint32_t stride_in_bytes, void*& out_data, D3D12_INDEX_BUFFER_VIEW& out_view)
{
    auto const size = mem::align_offset(num_indices * stride_in_bytes, 256);

    uint32_t memOffset;
    if (mMemoryRing.alloc(size, &memOffset) == false)
        return false;

    out_data = static_cast<void*>(mData + memOffset);

    out_view.BufferLocation = mResource->GetGPUVirtualAddress() + memOffset;
    out_view.Format = (stride_in_bytes == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    out_view.SizeInBytes = size;

    return true;
}

//--------------------------------------------------------------------------------------
//
// OnBeginFrame
//
//--------------------------------------------------------------------------------------
void DynamicBufferRing::onBeginFrame() { mMemoryRing.onBeginFrame(); }
}
