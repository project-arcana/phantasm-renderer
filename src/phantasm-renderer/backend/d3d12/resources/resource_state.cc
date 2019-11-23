#include "resource_state.hh"

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
