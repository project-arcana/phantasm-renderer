#pragma once

#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/cache_map.hh>
#include <phantasm-renderer/backend/detail/hash.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#include <phantasm-renderer/backend/vulkan/shader_arguments.hh>

namespace pr::backend::vk
{
/// Persistent cache for render passes
/// Unsynchronized, only used inside of pipeline pool
class RenderPassCache
{
public:
    void initialize(unsigned max_elements);
    void destroy(VkDevice device);

    /// receive an existing render pass matching the framebuffer formats and config, or create a new one
    [[nodiscard]] VkRenderPass getOrCreate(VkDevice device, arg::framebuffer_format rt_formats, pr::primitive_pipeline_config const& prim_conf);

    /// destroys all elements inside, and clears the map
    void reset(VkDevice device);

private:
    static size_t hashKey(arg::framebuffer_format rt_formats, pr::primitive_pipeline_config const& prim_conf)
    {
        return hash::detail::hash_combine(hash::compute(rt_formats), hash::compute(prim_conf));
    }

    backend::detail::cache_map<VkRenderPass> mCache;
};

}
