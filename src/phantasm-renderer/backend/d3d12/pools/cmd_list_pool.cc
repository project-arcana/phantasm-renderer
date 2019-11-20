#include "cmd_list_pool.hh"

#include <phantasm-renderer/backend/d3d12/BackendD3D12.hh>
#include <phantasm-renderer/backend/d3d12/common/verify.hh>

void pr::backend::d3d12::cmd_allocator_node::initialize(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type, int max_num_cmdlists)
{
    _max_num_in_flight = max_num_cmdlists;
    _submit_counter = 0;
    _submit_counter_at_last_reset = 0;
    _num_in_flight = 0;
    _num_discarded = 0;
    _full_and_waiting = false;

    _fence.initialize(device, "cmd_list allocator_node fence");
    PR_D3D12_VERIFY(device.CreateCommandAllocator(type, IID_PPV_ARGS(&_allocator)));
}

void pr::backend::d3d12::cmd_allocator_node::destroy() { _allocator->Release(); }

void pr::backend::d3d12::cmd_allocator_node::acquire(ID3D12GraphicsCommandList* cmd_list)
{
    if (is_full())
    {
        // the allocator is full, we are almost dead but might be able to reset
        auto const reset_success = try_reset_blocking();
        CC_RUNTIME_ASSERT(reset_success && "cmdlist allocator node overcommitted and unable to recover");
        // we were able to recover, but should warn even now
    }

    PR_D3D12_VERIFY(cmd_list->Reset(_allocator, nullptr));
    ++_num_in_flight;

    if (_num_in_flight == _max_num_in_flight)
        _full_and_waiting = true;
}

bool pr::backend::d3d12::cmd_allocator_node::try_reset()
{
    if (can_reset())
    {
        // full, and all acquired cmdlists have been either submitted or discarded, check the fence

        auto const fence_current = _fence.getCurrentValue();
        CC_ASSERT(fence_current <= _submit_counter && "fence counter overflow, did application run > 400 million years?");
        if (fence_current == _submit_counter)
        {
            // can reset, and the fence has reached its goal
            do_reset();
            return true;
        }
        else
        {
            // can reset, but the fence hasn't reached its goal yet
            return false;
        }
    }
    else
    {
        // can't reset
        return !is_full();
    }
}

bool pr::backend::d3d12::cmd_allocator_node::try_reset_blocking()
{
    if (can_reset())
    {
        // full, and all acquired cmdlists have been either submitted or discarded, wait for the fence
        _fence.waitCPU(_submit_counter);
        do_reset();
        return true;
    }
    else
    {
        // can't reset
        return !is_full();
    }
}

void pr::backend::d3d12::cmd_allocator_node::do_reset()
{
    PR_D3D12_VERIFY(_allocator->Reset());
    _full_and_waiting = false;
    _num_in_flight = 0;
    _num_discarded = 0;
    _submit_counter_at_last_reset = _submit_counter;
}

bool pr::backend::d3d12::cmd_allocator_node::is_submit_counter_up_to_date() const
{
    // Check if all lists acquired from this allocator
    // since the last reset have been either submitted or discarded
    int const submits_since_reset = static_cast<int>(_submit_counter - _submit_counter_at_last_reset);

    // this assert is paranoia-tier
    CC_ASSERT(submits_since_reset >= 0 && submits_since_reset <= _num_in_flight - _num_discarded);

    // if this condition is false, there have been less submits than acquired lists (minus the discarded ones)
    // so some are still pending submit (or discardation [sic])
    // we cannot check the fence yet since _submit_counter is currently meaningless
    return (submits_since_reset == _num_in_flight - _num_discarded);
}

pr::backend::handle::command_list pr::backend::d3d12::CommandListPool::create()
{
    unsigned res_handle;
    {
        auto lg = std::lock_guard(mMutex);
        findNextAllocatorIndex();
        res_handle = mPool.acquire();
        mAllocators[mActiveAllocator].acquire(mRawLists[res_handle]);
    }

    cmd_list_node& new_node = mPool.get(res_handle);
    new_node.responsible_allocator = &mAllocators[mActiveAllocator];

    return {static_cast<handle::index_t>(res_handle)};
}

void pr::backend::d3d12::CommandListPool::initialize(pr::backend::d3d12::BackendD3D12& backend, int num_allocators, int num_cmdlists_per_allocator)
{
    // NOTE: Eventually, we'll need multiple
    constexpr auto list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    auto const num_cmdlists_total = static_cast<size_t>(num_allocators * num_cmdlists_per_allocator);

    // initialize data structures
    mPool.initialize(num_cmdlists_total);
    mRawLists = mRawLists.uninitialized(num_cmdlists_total);
    mAllocators = mAllocators.defaulted(static_cast<size_t>(num_allocators));
    mActiveAllocator = 0u;

    auto& direct_queue = backend.mDirectQueue.getQueue();
    auto& device = backend.mDevice.getDevice();

    // Initialize allocators, create command lists
    {
        auto cmdlist_i = 0u;
        for (cmd_allocator_node& alloc_node : mAllocators)
        {
            alloc_node.initialize(device, list_type, num_cmdlists_per_allocator);
            ID3D12CommandAllocator* const raw_alloc = alloc_node.get_allocator();

            for (auto _ = 0; _ < num_cmdlists_per_allocator; ++_)
            {
                PR_D3D12_VERIFY(device.CreateCommandList(0, list_type, raw_alloc, nullptr, IID_PPV_ARGS(&mRawLists[cmdlist_i])));
                mRawLists[cmdlist_i]->Close();
                ++cmdlist_i;
            }
        }
    }

    // Execute all (closed) lists once to suppress warnings
    direct_queue.ExecuteCommandLists(                                 //
        UINT(mRawLists.size()),                                       //
        reinterpret_cast<ID3D12CommandList* const*>(mRawLists.data()) // This is hopefully always fine
    );

    // Flush the backend
    backend.flushGPU();
}

void pr::backend::d3d12::CommandListPool::destroy()
{
    for (auto const list : mRawLists)
    {
        list->Release();
    }

    for (cmd_allocator_node& alloc_node : mAllocators)
    {
        auto const reset_success = alloc_node.try_reset_blocking();
        CC_RUNTIME_ASSERT(reset_success);
        alloc_node.destroy();
    }
}

void pr::backend::d3d12::CommandListPool::findNextAllocatorIndex()
{
    for (auto it = 0u; it < mAllocators.size(); ++it)
    {
        if (!mAllocators[mActiveAllocator].is_full() || mAllocators[mActiveAllocator].try_reset())
            // not full, or nonblocking reset successful
            return;
        else
        {
            ++mActiveAllocator;
            // fast %
            if (mActiveAllocator >= mAllocators.size())
                mActiveAllocator -= mAllocators.size();
        }
    }

    // all non-blocking resets failed, try blocking now
    for (auto it = 0u; it < mAllocators.size(); ++it)
    {
        if (mAllocators[mActiveAllocator].try_reset_blocking())
            // blocking reset successful
            return;
        else
        {
            ++mActiveAllocator;
            // fast %
            if (mActiveAllocator >= mAllocators.size())
                mActiveAllocator -= mAllocators.size();
        }
    }

    // all allocators have at least 1 dangling cmdlist, we cannot recover
    CC_RUNTIME_ASSERT(false && "all allocators overcommitted and unresettable");
}