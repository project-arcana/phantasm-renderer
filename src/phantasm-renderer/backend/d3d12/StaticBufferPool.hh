#pragma once

#include <mutex>

#include "common/d3d12_sanitized.hh"
#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
// Simulates DX11 style static buffers. For dynamic buffers please see 'DynamicBufferRingDX12.h'
//
// This class allows suballocating small chuncks of memory from a huge buffer that is allocated on creation
// This class is specialized in vertex buffers.
//
class StaticBufferPool
{
public:
    void initialize(ID3D12Device& pDevice, uint32_t totalMemSize, bool bUseVidMem, const char* name);

    bool allocBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, D3D12_GPU_VIRTUAL_ADDRESS* pBufferLocation, uint32_t* pSize);
    bool allocBuffer(uint32_t numbeOfElements, uint32_t strideInBytes, void* pInitData, D3D12_GPU_VIRTUAL_ADDRESS* pBufferLocation, uint32_t* pSize);

    bool allocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, D3D12_VERTEX_BUFFER_VIEW* pView);
    bool allocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void** pData, D3D12_INDEX_BUFFER_VIEW* pIndexView);
    bool allocConstantBuffer(uint32_t size, void** pData, D3D12_CONSTANT_BUFFER_VIEW_DESC* pViewDesc);

    bool allocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void* pInitData, D3D12_VERTEX_BUFFER_VIEW* pOut);
    bool allocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void* pInitData, D3D12_INDEX_BUFFER_VIEW* pOut);
    bool allocConstantBuffer(uint32_t size, void* pData, D3D12_CONSTANT_BUFFER_VIEW_DESC* pViewDesc);

    void uploadData(ID3D12GraphicsCommandList* pCmdList);
    void freeUploadHeap();

private:
    shared_com_ptr<ID3D12Resource> mBufferMemory;
    shared_com_ptr<ID3D12Resource> mBufferSystemMemory;
    shared_com_ptr<ID3D12Resource> mBufferVideoMemory;
    char* mData = nullptr;
    uint32_t mMemoryInit = 0;
    uint32_t mMemoryOffset = 0;
    uint32_t mTotalSize = 0;
    bool mUseVideoMemory = true;
    std::mutex mMutex = {};
};
}
