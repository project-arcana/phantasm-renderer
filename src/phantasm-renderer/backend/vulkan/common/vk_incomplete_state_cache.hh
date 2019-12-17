#pragma once

#include <clean-core/capped_vector.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

#include "native_enum.hh"

namespace pr::backend::vk
{
struct vk_incomplete_state_cache
{
public:
    /// signal a resource transition to a given state
    /// returns true if the before state is known, or false otherwise
    bool transition_resource(handle::resource res, resource_state after, resource_state& out_before, VkPipelineStageFlags& out_before_dependency)
    {
        auto const after_dependencies = util::to_pipeline_stage_dependency(after, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        for (auto& entry : cache)
        {
            if (entry.ptr == res)
            {
                // resource is in cache
                out_before = entry.current;
                out_before_dependency = entry.current_shader_dependency;
                entry.current = after;
                entry.current_shader_dependency = after_dependencies;
                return true;
            }
        }

        cache.push_back({res, after, after, after_dependencies, after_dependencies});
        return false;
    }

    void touch_resource_in_shader(handle::resource res, VkPipelineStageFlags shader_pipeline_flags)
    {
        for (auto& entry : cache)
        {
            if (entry.ptr == res)
            {
                // resource is in cache

                if (entry.initial_shader_dependency == VK_PIPELINE_STAGE_HOST_BIT)
                {
                    // this entry has not yet been written to and requires updating
                    entry.initial_shader_dependency = shader_pipeline_flags;
                }

                entry.current_shader_dependency = shader_pipeline_flags;
                return;
            }
        }

        // not in cache, this is not necessarily an error
        // (user blindly trusts resource to be in the right state before using this resource and will not
        // use another barrier on it within this command buffer)
    }


    void reset() { cache.clear(); }

public:
    struct cache_entry
    {
        /// (const) the resource handle
        handle::resource ptr;
        /// (const) the <after> state of the initial barrier (<before> is unknown)
        resource_state required_initial;
        /// latest state of this resource
        resource_state current;
        /// the first pipeline stage touching this resource
        VkPipelineStageFlags initial_shader_dependency;
        /// the latest pipeline stage to touch this resource
        VkPipelineStageFlags current_shader_dependency;
    };

    // linear "map" for now, might want to benchmark this
    cc::capped_vector<cache_entry, 32> cache;
};
}
