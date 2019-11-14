#pragma once

#include <clean-core/array.hh>

#include "common/d3d12_sanitized.hh"

#include "common/shared_com_ptr.hh"
#include "common/verify.hh"

namespace pr::backend::d3d12
{
class BackendD3D12;

/// Ring buffer of command allocators, each with N command lists
/// Unsynchronized
class CommandListRing
{
public:
    void initialize(BackendD3D12& backend, unsigned num_allocators, unsigned num_command_lists_per_allocator, D3D12_COMMAND_LIST_TYPE type);

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
