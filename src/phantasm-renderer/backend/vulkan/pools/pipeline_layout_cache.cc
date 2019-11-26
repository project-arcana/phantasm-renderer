#include "pipeline_layout_cache.hh"

void pr::backend::vk::PipelineLayoutCache::initialize(unsigned size_estimate) { mCache.reserve(size_estimate); }

void pr::backend::vk::PipelineLayoutCache::destroy(VkDevice device) { reset(device); }

pr::backend::vk::pipeline_layout* pr::backend::vk::PipelineLayoutCache::getOrCreate(VkDevice device, arg::shader_argument_shapes arg_shapes)
{
    if (auto const it = mCache.find(arg_shapes); it != mCache.end())
    {
        return &it->second;
    }
    else
    {
        auto [new_it, success] = mCache.emplace(arg_shapes, pipeline_layout{});
        CC_ASSERT(success);
        new_it->second.initialize(device, arg_shapes);
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
