#pragma once
#ifdef PR_BACKEND_D3D12

#include <atomic>

#include <d3d12.h>

#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
/// Represents a ID3D12CommandAllocator
class CommandAllocator
{
    // reference type
public:
    CommandAllocator() = default;
    CommandAllocator(CommandAllocator const&) = delete;
    CommandAllocator& operator=(CommandAllocator const&) = delete;

private:
    shared_com_ptr<ID3D12CommandAllocator> mCommandAllocator;
    std::atomic_int mNumCommandListsInFlight;

private:
};

class CommandList
{

    void foo()
    {
        mCommandList->Close();
    }

private:
    shared_com_ptr<ID3D12GraphicsCommandList> mCommandList;
    shared_com_ptr<ID3D12GraphicsCommandList1> mCommandList1;
    shared_com_ptr<ID3D12GraphicsCommandList4> mCommandListRaytracing;
};

}

#endif
