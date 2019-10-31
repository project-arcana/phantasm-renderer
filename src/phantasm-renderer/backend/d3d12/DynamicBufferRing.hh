#pragma once

#include "ResourceViewHeaps.hh"

#include "common/shared_com_ptr.hh"
#include "memory/Ring.hh"

namespace pr::backend::d3d12
{
// This class mimics the behaviour or the DX11 dynamic buffers.
// It does so by suballocating memory from a huge buffer. The buffer is used in a ring fashion.
// Allocated memory is taken from the tail, freed memory makes the head advance;
// See 'ring.h' to get more details on the ring buffer.
//
// The class knows when to free memory by just knowing:
//    1) the amount of memory used per frame
//    2) the number of backbuffers
//    3) When a new frame just started ( indicated by OnBeginFrame() )
//         - This will free the data of the oldest frame so it can be reused for the new frame
//
// Note than in this ring an allocated chuck of memory has to be contiguous in memory, that is it cannot spawn accross the tail and the head.
// This class takes care of that.

class DynamicBufferRing
{
public:
    void initialize(ID3D12Device& pDevice, uint32_t numberOfBackBuffers, uint32_t memTotalSize);

    void onBeginFrame();

    bool allocIndexBuffer(uint32_t numbeOfIndices, uint32_t strideInBytes, void** pData, D3D12_INDEX_BUFFER_VIEW* pView);
    bool allocVertexBuffer(uint32_t numbeOfVertices, uint32_t strideInBytes, void** pData, D3D12_VERTEX_BUFFER_VIEW* pView);
    bool allocConstantBuffer(uint32_t size, void** pData, D3D12_GPU_VIRTUAL_ADDRESS* pBufferViewDesc);

private:
    shared_com_ptr<ID3D12Resource> mResource;
    char* mData = nullptr;
    RingWithTabs mMemoryRing;
    uint32_t mTotalSize;
};
}
