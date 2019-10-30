#pragma once
#ifdef PR_BACKEND_D3D12

#include <atomic>

#include <clean-core/move.hh>

#include "common/d3d12_sanitized.hh"

#include "common/shared_com_ptr.hh"
#include "common/verify.hh"

namespace pr::backend::d3d12
{
class CommandAllocator;

struct CommandList
{
    shared_com_ptr<ID3D12CommandList> ptr;
};

/// Represents a ID3D12CommandAllocator
class CommandAllocator
{
    // reference type
public:
    CommandAllocator(shared_com_ptr<ID3D12Device> parent_device) : mParentDevice(cc::move(parent_device))
    {
        PR_D3D12_VERIFY(mParentDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, PR_COM_WRITE(mCommandAllocator)));
    }

    CommandAllocator(CommandAllocator const&) = delete;
    CommandAllocator& operator=(CommandAllocator const&) = delete;

    [[nodiscard]] CommandList createCommandList()
    {
        CommandList res;
        PR_D3D12_VERIFY(mParentDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator, nullptr, PR_COM_WRITE(res.ptr)));
        return res;
    }

    void reset()
    {
        // This fails if any command list created by this allocator is still alive
        PR_D3D12_VERIFY(mCommandAllocator->Reset());
    }

private:
    shared_com_ptr<ID3D12Device> const mParentDevice;
    shared_com_ptr<ID3D12CommandAllocator> mCommandAllocator;

private:
};
}

#endif
