#include "pipeline_layout_cache.hh"

void pr::backend::vk::PipelineLayoutCache::initialize(unsigned size_estimate) { mCache.reserve(size_estimate); }

void pr::backend::vk::PipelineLayoutCache::destroy(VkDevice device) { reset(device); }

pr::backend::vk::pipeline_layout* pr::backend::vk::PipelineLayoutCache::getOrCreate(VkDevice device, key_t key)
{
    if (auto const it = mCache.find(key); it != mCache.end())
    {
        return &it->second;
    }
    else
    {
        auto [new_it, success] = mCache.emplace(key, pipeline_layout{});
        CC_ASSERT(success);
        new_it->second.initialize(device, key.reflected_ranges, key.arg_samplers);
        return &new_it->second;
    }
}

void pr::backend::vk::PipelineLayoutCache::reset(VkDevice device)
{
    for (auto& elem : mCache)
    {
        elem.second.free(device);
    }
    mCache.clear();
}
