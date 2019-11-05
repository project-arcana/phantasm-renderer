#pragma once

#include <atomic>

#include <clean-core/array.hh>
#include <clean-core/move.hh>

#include "common/d3d12_sanitized.hh"

#include "common/shared_com_ptr.hh"
#include "common/verify.hh"

namespace pr::backend::d3d12
{
class BackendD3D12;

struct CommandList
{
    explicit CommandList(ID3D12GraphicsCommandList* list) : mType(D3D12_COMMAND_LIST_TYPE_DIRECT), mCommandList(list) { cc::fill(mDescriptorHeaps, nullptr); }

    void setDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap);

    void close();

public:
    [[nodiscard]] D3D12_COMMAND_LIST_TYPE const& getType() const { return mType; }

    [[nodiscard]] ID3D12GraphicsCommandList& ref() const { return *mCommandList; }
    [[nodiscard]] ID3D12GraphicsCommandList* ptr() const { return mCommandList; }

private:
    void rebindDescriptorHeaps();

private:
    /// The type of this command list
    D3D12_COMMAND_LIST_TYPE mType;

    /// Raw, non-owning pointer
    ID3D12GraphicsCommandList* mCommandList;

    /// Weak references to the descriptor heaps that are currently bound
    cc::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> mDescriptorHeaps;
};

/// Ring buffer of command allocators, each with N command lists
/// Unsynchronized
class CommandListRing
{
public:
    void initialize(BackendD3D12& backend, unsigned num_allocators, unsigned num_command_lists_per_allocator, D3D12_COMMAND_QUEUE_DESC const& queue_desc);

    /// Steps forward in the ring, resetting the next allocator
    void onBeginFrame();

    /// Acquires a command list
    [[nodiscard]] ID3D12GraphicsCommandList* acquireCommandList();

private:
    struct ring_entry
    {
        shared_com_ptr<ID3D12CommandAllocator> command_allocator;
        cc::array<shared_com_ptr<ID3D12GraphicsCommandList>> command_lists;
        int num_command_lists_in_flight = 0;

        [[nodiscard]] ID3D12GraphicsCommandList* acquire_list();
        void reset();
    };

    cc::array<ring_entry> mRing;
    int mRingIndex = 0;
    ring_entry* mActiveEntry = nullptr;
};
}
