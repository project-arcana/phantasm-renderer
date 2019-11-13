#pragma once

#include <clean-core/capped_array.hh>
#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>
#include <phantasm-renderer/backend/d3d12/common/util.hh>

namespace pr::backend::d3d12
{
/// A thread-local, incomplete-information resource state cache
/// Keeps track of locally known resource states, and stores the required initial states
/// After use:
///     1. command list and incomplete state cache are passed to submission thread
///     2. submission thread creates an additional, small command list to be executed first
///     3. goes through the master state cache to find all the unknown <before> states
///     4. creates barriers for all cache entries, transitioning from (known) <before> to cache_entry::required_initial
///     5. executes small "barrier" command list, then executes the proper command list, now with all states correctly in place
///     6. updates master cache with all the cache_entry::current states
struct incomplete_state_cache
{
public:
    /// signal a resource transition to a given state
    /// returns true if the before state is known, or false otherwise
    bool transition_resource(ID3D12Resource* resource, D3D12_RESOURCE_STATES after, D3D12_RESOURCE_STATES& out_before)
    {
        bool found = false;

        for (auto& entry : cache)
        {
            if (entry.ptr == resource)
            {
                found = true;
                out_before = entry.current;
                entry.current = after;
                break;
            }
        }

        if (!found)
            cache.push_back({resource, after, after});

        return found;
    }

    /// all-in-one convenience, submitting the barrier itself if applicable
    /// note that this version should eventually be removed, in favor of batched, split barriers
    void transition_and_barrier_resource(ID3D12GraphicsCommandList* command_list, ID3D12Resource* res, D3D12_RESOURCE_STATES after)
    {
        D3D12_RESOURCE_STATES before;
        if (transition_resource(res, after, before))
        {
            // this resource is already known, submit a regular barrier
            util::transition_barrier(command_list, res, before, after);
        }
        else
        {
            // do nothing, this barrier will be taken care of upon submission
        }
    }

public:
    struct cache_entry
    {
        /// (const) the raw resource pointer, serves only as a key
        ID3D12Resource* ptr;
        /// (const) the <after> state of the initial barrier (<before> is unknown)
        D3D12_RESOURCE_STATES required_initial;
        /// latest state of this resource
        D3D12_RESOURCE_STATES current;
    };

    // linear "map" for now, might want to benchmark this
    cc::capped_vector<cache_entry, 32> cache;
};

/// A single state cache with the definitive information about each resource's state
/// responsible for filling small command lists with nothing but barriers so that assumptions
/// made by threads with incomplete_state_caches hold in their command lists
struct master_state_cache
{
    /// submits all required resource barriers based on an incomplete state cache
    /// also updates the master cache to the listed current states
    void submit_required_incomplete_barriers(ID3D12GraphicsCommandList* command_list, incomplete_state_cache const& incomplete_cache)
    {
        cc::capped_array<D3D12_RESOURCE_BARRIER, 32> barriers;
        barriers.emplace(incomplete_cache.cache.size());

        for (auto i = 0u; i < incomplete_cache.cache.size(); ++i)
        {
            auto const& entry = incomplete_cache.cache[i];

            auto const before = get_master_state(entry.ptr);

            // transition to the state required as the initial one
            util::populate_barrier_desc(barriers[i], entry.ptr, before, entry.required_initial);
            // set the master state to the one in which this resource is left
            set_master_state(entry.ptr, entry.current);
        }

        command_list->ResourceBarrier(UINT(barriers.size()), barriers.data());
    }

private:
    D3D12_RESOURCE_STATES get_master_state(ID3D12Resource* resource) const
    {
        // TODO
        return D3D12_RESOURCE_STATE_COMMON;
    }

    void set_master_state(ID3D12Resource* resource, D3D12_RESOURCE_STATES state)
    {
        // TODO
    }
};
}
