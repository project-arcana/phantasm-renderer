#pragma once
#ifdef PR_BACKEND_D3D12

#include <atomic>

#include <clean-core/array.hh>
#include <clean-core/move.hh>

#include "common/d3d12_sanitized.hh"

#include "common/shared_com_ptr.hh"
#include "common/verify.hh"

namespace pr::backend::d3d12
{
class CommandAllocator;

struct CommandList
{
    CommandList() { cc::fill(mDescriptorHeaps, nullptr); }

    void initialize(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator& allocator);

    void setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap);

    void close();

public:
    [[nodiscard]] D3D12_COMMAND_LIST_TYPE const& getType() const { return mType; }

    [[nodiscard]] ID3D12GraphicsCommandList4& getCommandList() const { return *mCommandList.get(); }
    [[nodiscard]] shared_com_ptr<ID3D12GraphicsCommandList4> const& getCommandListShared() const { return mCommandList; }

private:
    void rebindDescriptorHeaps();

private:
    /// The type of this command list
    D3D12_COMMAND_LIST_TYPE mType;
    shared_com_ptr<ID3D12GraphicsCommandList4> mCommandList;

    /// Weak references to the descriptor heaps that are currently bound
    cc::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mDescriptorHeaps;
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
        res.initialize(*mParentDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, *mCommandAllocator);
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
