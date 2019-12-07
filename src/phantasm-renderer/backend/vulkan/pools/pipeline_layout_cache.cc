#include "pipeline_layout_cache.hh"

void pr::backend::vk::PipelineLayoutCache::initialize(unsigned max_elements) { mCache.initialize(max_elements); }

void pr::backend::vk::PipelineLayoutCache::destroy(VkDevice device) { reset(device); }

pr::backend::vk::pipeline_layout* pr::backend::vk::PipelineLayoutCache::getOrCreate(VkDevice device, key_t key)
{
    auto const hash = hashKey(key);

    auto* const lookup = mCache.look_up(hash);
    if (lookup != nullptr)
        return lookup;
    else
    {
        auto* const insertion = mCache.insert(hash, pipeline_layout{});
        insertion->initialize(device, key.reflected_ranges);
        return insertion;
    }
}

void pr::backend::vk::PipelineLayoutCache::reset(VkDevice device)
{
    mCache.iterate_elements([&](pipeline_layout& elem) { elem.free(device); });
    mCache.clear();
}
