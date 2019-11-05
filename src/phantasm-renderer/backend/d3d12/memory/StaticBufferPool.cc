#include "StaticBufferPool.hh"

#include <clean-core/assert.hh>

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

#include "byte_util.hh"

namespace pr::backend::d3d12
{
void StaticBufferPool::initialize(ID3D12Device& pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name)
{
    mTotalSize = totalMemSize;
    mUseVideoMemory = bUseVidMem;

    if (bUseVidMem)
    {
        auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto const resource_desc = CD3DX12_RESOURCE_DESC::Buffer(totalMemSize);
        PR_D3D12_VERIFY(pDevice.CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                                        PR_COM_WRITE(mBufferVideoMemory)));
        mBufferVideoMemory->SetName(L"StaticBufferPoolDX12::m_pVidMemBuffer");
    }

    {
        auto const heap_props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto const resource_desc = CD3DX12_RESOURCE_DESC::Buffer(totalMemSize);
        PR_D3D12_VERIFY(pDevice.CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                        PR_COM_WRITE(mBufferSystemMemory)));
        mBufferSystemMemory->SetName(L"StaticBufferPoolDX12::m_pSysMemBuffer");
        mBufferSystemMemory->Map(0, nullptr, reinterpret_cast<void**>(&mData));
    }
}

bool StaticBufferPool::allocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void** pData, D3D12_GPU_VIRTUAL_ADDRESS* pBufferLocation, uint32_t* pSize)
{
    std::lock_guard<std::mutex> lock(mMutex);

    auto const size = mem::align_offset(numbeOfElements * strideInBytes, 256);
    CC_ASSERT(mMemoryOffset + size < mTotalSize);

    *pData = (void*)(mData + mMemoryOffset);

    *pBufferLocation = mMemoryOffset + ((mUseVideoMemory) ? mBufferVideoMemory->GetGPUVirtualAddress() : mBufferSystemMemory->GetGPUVirtualAddress());
    *pSize = size;

    mMemoryOffset += size;

    return true;
}

bool StaticBufferPool::allocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void* pInitData, D3D12_GPU_VIRTUAL_ADDRESS* pBufferLocation, uint32_t* pSize)
{
    void* data;
    if (allocBuffer(numbeOfElements, strideInBytes, &data, pBufferLocation, pSize))
    {
        memcpy(data, pInitData, numbeOfElements * strideInBytes);
        return true;
    }
    return false;
}

bool StaticBufferPool::allocVertexBuffer(uint32_t numberOfVertices, uint32_t strideInBytes, void** pData, D3D12_VERTEX_BUFFER_VIEW* pView)
{
    allocBuffer(numberOfVertices, strideInBytes, pData, &pView->BufferLocation, &pView->SizeInBytes);
    pView->StrideInBytes = strideInBytes;

    return true;
}

bool StaticBufferPool::allocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void* pInitData, D3D12_VERTEX_BUFFER_VIEW* pOut)
{
    void* data;
    if (allocVertexBuffer(numbeOfVertices, strideInBytes, &data, pOut))
    {
        memcpy(data, pInitData, numbeOfVertices * strideInBytes);
        return true;
    }
    return false;
}

bool StaticBufferPool::allocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void** pData, D3D12_INDEX_BUFFER_VIEW* pView)
{
    allocBuffer(numbeOfIndices, strideInBytes, pData, &pView->BufferLocation, &pView->SizeInBytes);
    pView->Format = (strideInBytes == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;

    return true;
}

bool StaticBufferPool::allocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void* pInitData, D3D12_INDEX_BUFFER_VIEW* pOut)
{
    void* data;
    if (allocIndexBuffer(numbeOfIndices, strideInBytes, &data, pOut))
    {
        ::memcpy(data, pInitData, numbeOfIndices * strideInBytes);
        return true;
    }
    return false;
}

bool StaticBufferPool::allocConstantBuffer(uint32_t size, void** data, D3D12_CONSTANT_BUFFER_VIEW_DESC* pViewDesc)
{
    allocBuffer(size, 1, data, &pViewDesc->BufferLocation, &pViewDesc->SizeInBytes);
    return true;
}

bool StaticBufferPool::allocConstantBuffer(uint32_t size, void* init_data, D3D12_CONSTANT_BUFFER_VIEW_DESC* pOut)
{
    void* data;
    if (allocConstantBuffer(size, &data, pOut))
    {
        ::memcpy(data, init_data, size);
        return true;
    }
    return false;
}

void StaticBufferPool::uploadData(ID3D12GraphicsCommandList* cmd_list)
{
    if (mUseVideoMemory)
    {
        cmd_list->CopyBufferRegion(mBufferVideoMemory, mMemoryInit, mBufferSystemMemory, mMemoryInit, mMemoryOffset - mMemoryInit);

        // With 'dynamic resources' we can use a same resource to hold Constant, Index and Vertex buffers.
        // That is because we dont need to use a transition.
        //
        // With static buffers though we need to transition them and we only have 2 options
        //      1) D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        //      2) D3D12_RESOURCE_STATE_INDEX_BUFFER
        // Because we need to transition the whole buffer we cant have now Index buffers to share the
        // same resource with the Vertex or Constant buffers. Hence is why we need separate classes.
        // For Index and Vertex buffers we *could* use the same resource, but index buffers need their own resource.
        // Please note that in the interest of clarity vertex buffers and constant buffers have been split into two different classes though
        auto const barrier
            = CD3DX12_RESOURCE_BARRIER::Transition(mBufferVideoMemory, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        cmd_list->ResourceBarrier(1, &barrier);

        mMemoryInit = mMemoryOffset;
    }
}

void StaticBufferPool::freeUploadHeap()
{
    // remember this is a shared_com_ptr
    mBufferSystemMemory = nullptr;
}
}
