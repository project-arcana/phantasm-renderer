#include "resource_state.hh"

#include <phantasm-renderer/backend/d3d12/common/util.hh>
#include <phantasm-renderer/backend/d3d12/memory/D3D12MA.hh>

bool pr::backend::d3d12::incomplete_state_cache::transition_resource(D3D12MA::Allocation* allocation, D3D12_RESOURCE_STATES after, D3D12_RESOURCE_STATES& out_before)
{
    bool found = false;

    for (auto& entry : cache)
    {
        if (entry.ptr == allocation)
        {
            found = true;
            out_before = entry.current;
            entry.current = after;
            break;
        }
    }

    if (!found)
        cache.push_back({allocation, after, after});

    return found;
}

void pr::backend::d3d12::incomplete_state_cache::transition_and_barrier_resource(ID3D12GraphicsCommandList* command_list,
                                                                                 D3D12MA::Allocation* allocation,
                                                                                 D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_STATES before;
    if (transition_resource(allocation, after, before))
    {
        // subsequent barrier
        // this resource is already known, submit a regular barrier
        util::transition_barrier(command_list, allocation->GetResource(), before, after);
    }
    else
    {
        // initial barrier
        // do nothing, this barrier will be taken care of upon submission
    }
}

void pr::backend::d3d12::master_state_cache::submit_required_incomplete_barriers(ID3D12GraphicsCommandList* command_list,
                                                                                 const pr::backend::d3d12::incomplete_state_cache& incomplete_cache)
{
    cc::capped_vector<D3D12_RESOURCE_BARRIER, 32> barriers;

    for (auto const& entry : incomplete_cache.cache)
    {
        auto const before = entry.ptr->pr_getResourceState();

        if (before != entry.required_initial)
        {
            // transition to the state required as the initial one
            auto& barrier = barriers.emplace_back();
            util::populate_barrier_desc(barrier, entry.ptr->GetResource(), before, entry.required_initial);
        }

        // set the master state to the one in which this resource is left
        entry.ptr->pr_setResourceState(entry.current);
    }

    command_list->ResourceBarrier(UINT(barriers.size()), barriers.data());
}

void pr::backend::d3d12::master_state_cache::submit_initial_creation_barriers(ID3D12GraphicsCommandList* command_list,
                                                                              cc::span<const pr::backend::d3d12::master_state_cache::init_state> init_states)
{
    cc::capped_vector<D3D12_RESOURCE_BARRIER, 32> barriers;

    for (auto const& entry : init_states)
    {
        // perform the initial state barrier
        auto& barrier = barriers.emplace_back();
        util::populate_barrier_desc(barrier, entry.ptr->GetResource(), entry.ptr->pr_getResourceState(), entry.initial);

        // set the master state to the initial one
        entry.ptr->pr_setResourceState(entry.initial);
    }

    command_list->ResourceBarrier(UINT(barriers.size()), barriers.data());
}
