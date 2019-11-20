#include "cmd_list_pool.hh"

#include <phantasm-renderer/backend/d3d12/common/verify.hh>

void pr::backend::d3d12::allocator_node::initialize(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type, int max_num_cmdlists)
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

void pr::backend::d3d12::allocator_node::destroy() { _allocator->Release(); }

void pr::backend::d3d12::allocator_node::acquire(ID3D12GraphicsCommandList* cmd_list)
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

bool pr::backend::d3d12::allocator_node::try_reset()
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

bool pr::backend::d3d12::allocator_node::try_reset_blocking()
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

void pr::backend::d3d12::allocator_node::do_reset()
{
    PR_D3D12_VERIFY(_allocator->Reset());
    _full_and_waiting = false;
    _num_in_flight = 0;
    _num_discarded = 0;
    _submit_counter_at_last_reset = _submit_counter;
}

bool pr::backend::d3d12::allocator_node::is_submit_counter_up_to_date() const
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
