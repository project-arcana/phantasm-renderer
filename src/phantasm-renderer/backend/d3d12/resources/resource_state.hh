#pragma once

#include <clean-core/capped_vector.hh>
#include <clean-core/span.hh>

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>

namespace D3D12MA
{
class Allocation;
}

namespace pr::backend::d3d12
{
///
/// The only way to transition resources is via either of these two classes
/// Only exceptions:
///     	- possible pre-init barriers for copying procedures
///         - swapchain resources (see Swapchain.hh)
///

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
    bool transition_resource(D3D12MA::Allocation* allocation, D3D12_RESOURCE_STATES after, D3D12_RESOURCE_STATES& out_before);

    /// all-in-one convenience, submitting the barrier itself if applicable
    /// note that this version should eventually be removed, in favor of batched, split barriers
    void transition_and_barrier_resource(ID3D12GraphicsCommandList* command_list, D3D12MA::Allocation* allocation, D3D12_RESOURCE_STATES after);

public:
    struct cache_entry
    {
        /// (const) the raw resource pointer
        D3D12MA::Allocation* ptr;
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
///
/// This class is stateless (no globals involved), since the master state is stored within D3D12MA allocations themselves
///
///
struct master_state_cache
{
    struct init_state
    {
        /// (const) the raw resource pointer
        D3D12MA::Allocation* ptr;
        /// (const) the <before> state of the initial barrier
        D3D12_RESOURCE_STATES before;
        /// (const) the <after> state of the initial barrier
        D3D12_RESOURCE_STATES initial;

        init_state() = default;
        init_state(D3D12MA::Allocation* p, D3D12_RESOURCE_STATES b, D3D12_RESOURCE_STATES i) : ptr(p), before(b), initial(i) {}
    };

    /// submits all required resource barriers based on an incomplete state cache
    /// updates the master cache to the current states
    static void submit_required_incomplete_barriers(ID3D12GraphicsCommandList* command_list, incomplete_state_cache const& incomplete_cache);

    /// submits all required resource barriers based on a span of init states
    /// updates the master cache to the initial states
    static void submit_initial_creation_barriers(ID3D12GraphicsCommandList* command_list, cc::span<init_state const> init_states);

    master_state_cache() = delete;
};
}
