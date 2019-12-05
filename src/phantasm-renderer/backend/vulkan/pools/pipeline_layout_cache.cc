#include "pipeline_layout_cache.hh"

void pr::backend::vk::PipelineLayoutCache::initialize(unsigned max_elements) { mCache.initialize(max_elements); }

void pr::backend::vk::PipelineLayoutCache::destroy(VkDevice device, DescriptorAllocator& desc_alloc) { reset(device, desc_alloc); }

pr::backend::vk::pipeline_layout* pr::backend::vk::PipelineLayoutCache::getOrCreate(VkDevice device, key_t key, DescriptorAllocator& desc_alloc)
{
    auto const hash = hashKey(key);

    auto* const lookup = mCache.look_up(hash);
    if (lookup != nullptr)
        return lookup;
    else
    {
        auto* const insertion = mCache.insert(hash, pipeline_layout{});
        insertion->initialize(device, key.reflected_ranges, key.arg_samplers, desc_alloc);
        return insertion;
    }
}

void pr::backend::vk::PipelineLayoutCache::reset(VkDevice device, DescriptorAllocator& desc_alloc)
{
    mCache.iterate_elements([&](pipeline_layout& elem) { elem.free(device, desc_alloc); });
    mCache.clear();
}
