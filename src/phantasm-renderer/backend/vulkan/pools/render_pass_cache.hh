#pragma once

#include <unordered_map>

#include <phantasm-renderer/primitive_pipeline_config.hh>

#include <phantasm-renderer/backend/arguments.hh>
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
    void initialize(unsigned size_estimate = 50);
    void destroy(VkDevice device);

    /// receive an existing render pass matching the framebuffer formats and config, or create a new one
    [[nodiscard]] VkRenderPass getOrCreate(VkDevice device, arg::framebuffer_format rt_formats, pr::primitive_pipeline_config const& prim_conf);

    /// destroys all elements inside, and clears the map
    void reset(VkDevice device);

private:
    struct key_t
    {
        arg::framebuffer_format rt_formats;
        pr::primitive_pipeline_config prim_conf;

        constexpr bool operator==(key_t const& rhs) const noexcept
        {
            return rt_formats == rhs.rt_formats
                   && (prim_conf.cull == rhs.prim_conf.cull && prim_conf.depth == rhs.prim_conf.depth & prim_conf.samples == rhs.prim_conf.samples
                       && prim_conf.topology == rhs.prim_conf.topology && prim_conf.depth == rhs.prim_conf.depth
                       && prim_conf.depth_readonly == rhs.prim_conf.depth_readonly);
        }
    };

    struct key_hasher_functor
    {
        std::size_t operator()(key_t const& v) const noexcept
        {
            return hash::detail::hash_combine(hash::compute(v.rt_formats), hash::compute(v.prim_conf));
        }
    };

    std::unordered_map<key_t, VkRenderPass, key_hasher_functor> mCache;
};

}
