#include "render_pass_cache.hh"

#include <phantasm-renderer/backend/vulkan/pipeline_state.hh>

void pr::backend::vk::RenderPassCache::initialize(unsigned max_elements) { mCache.initialize(max_elements); }

void pr::backend::vk::RenderPassCache::destroy(VkDevice device) { reset(device); }

VkRenderPass pr::backend::vk::RenderPassCache::getOrCreate(VkDevice device, pr::backend::arg::framebuffer_format rt_formats, const pr::primitive_pipeline_config& prim_conf)
{
    auto const hash = hashKey(rt_formats, prim_conf);

    auto* const lookup = mCache.look_up(hash);
    if (lookup != nullptr)
        return *lookup;
    else
    {
        auto const new_rp = create_render_pass(device, rt_formats, prim_conf);
        auto* const insertion = mCache.insert(hash, new_rp);
        return *insertion;
    }
}

void pr::backend::vk::RenderPassCache::reset(VkDevice device)
{
    mCache.iterate_elements([&](VkRenderPass& elem) { vkDestroyRenderPass(device, elem, nullptr); });

    mCache.clear();
}
