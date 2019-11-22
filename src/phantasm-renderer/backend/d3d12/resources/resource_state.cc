#include "resource_state.hh"

#include <phantasm-renderer/backend/d3d12/common/util.hh>
#include <phantasm-renderer/backend/d3d12/memory/D3D12MA.hh>

bool pr::backend::d3d12::incomplete_state_cache::transition_resource(handle::resource res, resource_state after, resource_state& out_before)
{
    for (auto& entry : cache)
    {
        if (entry.ptr == res)
        {
            // resource is in cache
            out_before = entry.current;
            entry.current = after;
            return true;
        }
    }

    cache.push_back({res, after, after});
    return false;
}

void pr::backend::d3d12::master_state_cache::submit_required_incomplete_barriers(ID3D12GraphicsCommandList* command_list,
                                                                                 const pr::backend::d3d12::incomplete_state_cache& incomplete_cache)
{
    cc::capped_vector<D3D12_RESOURCE_BARRIER, 32> barriers;

//    for (auto const& entry : incomplete_cache.cache)
//    {
//        auto const before = entry.ptr->pr_getResourceState();

//        if (before != to_resource_states(entry.required_initial))
//        {
//            // transition to the state required as the initial one
//            auto& barrier = barriers.emplace_back();
//            util::populate_barrier_desc(barrier, entry.ptr->GetResource(), before, to_resource_states(entry.required_initial));
//        }

//        // set the master state to the one in which this resource is left
//        entry.ptr->pr_setResourceState(to_resource_states(entry.current));
//    }

//    command_list->ResourceBarrier(UINT(barriers.size()), barriers.data());
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
