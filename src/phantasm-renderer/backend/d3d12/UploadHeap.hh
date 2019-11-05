#pragma once

#include <cstdint>

#include "common/d3d12_sanitized.hh"
#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
class BackendD3D12;

//
// This class shows the most efficient way to upload resources to the GPU memory.
// The idea is to create just one upload heap and suballocate memory from it.
// For convenience this class comes with its own command list & submit (flushAndFinish)
//
class UploadHeap
{
public:
    void initialize(BackendD3D12* backend, size_t size);

    [[nodiscard]] uint8_t* suballocate(size_t size, size_t alignment);

    template <class T>
    [[nodiscard]] uint8_t* suballocate()
    {
        return suballocate(sizeof(T), alignof(T));
    }

    void flushAndFinish();

    [[nodiscard]] uint8_t* getBasePointer() const { return mDataBegin; }
    [[nodiscard]] ID3D12Resource* getResource() const { return mUploadHeap; }
    [[nodiscard]] ID3D12GraphicsCommandList* getCommandList() const { return mCommandList; }


private:
    BackendD3D12* mBackend;
    shared_com_ptr<ID3D12Resource> mUploadHeap;
    shared_com_ptr<ID3D12GraphicsCommandList> mCommandList;
    shared_com_ptr<ID3D12CommandAllocator> mCommandAllocator;

    uint8_t* mDataCurrent = nullptr; // current position of upload heap
    uint8_t* mDataBegin = nullptr;   // starting position of upload heap
    uint8_t* mDataEnd = nullptr;     // ending position of upload heap
};
}