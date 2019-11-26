#include "render_pass_cache.hh"

#include <phantasm-renderer/backend/vulkan/pipeline_state.hh>

void pr::backend::vk::RenderPassCache::initialize(unsigned size_estimate) { mCache.reserve(size_estimate); }

void pr::backend::vk::RenderPassCache::destroy(VkDevice device) { reset(device); }

VkRenderPass pr::backend::vk::RenderPassCache::getOrCreate(VkDevice device, pr::backend::arg::framebuffer_format rt_formats, const pr::primitive_pipeline_config& prim_conf)
{
    key_t key{rt_formats, prim_conf};
    if (auto const it = mCache.find(key); it != mCache.end())
    {
        return it->second;
    }
    else
    {
        auto const new_rp = create_render_pass(device, rt_formats, prim_conf);

        auto success = mCache.insert({key, new_rp}).second;
        CC_ASSERT(success);
        return new_rp;
    }
}

void pr::backend::vk::RenderPassCache::reset(VkDevice device)
{
    for (auto& elem : mCache)
    {
        vkDestroyRenderPass(device, elem.second, nullptr);
    }
    mCache.clear();
}
