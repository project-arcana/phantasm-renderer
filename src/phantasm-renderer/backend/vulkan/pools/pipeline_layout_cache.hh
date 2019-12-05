#pragma once

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/cache_map.hh>
#include <phantasm-renderer/backend/detail/hash.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#include <phantasm-renderer/backend/vulkan/shader_arguments.hh>

namespace pr::backend::vk
{
class DescriptorAllocator;

/// Persistent cache for pipeline layouts
/// Unsynchronized, only used inside of pipeline pool
class PipelineLayoutCache
{
public:
    struct key_t
    {
        cc::span<util::spirv_desc_range_info const> reflected_ranges;
        arg::shader_sampler_configs arg_samplers;
    };

    void initialize(unsigned max_elements);
    void destroy(VkDevice device, DescriptorAllocator& desc_alloc);

    /// receive an existing root signature matching the shape, or create a new one
    /// returns a pointer (which remains stable, §23.2.5/13 C++11)
    [[nodiscard]] pipeline_layout* getOrCreate(VkDevice device, key_t key, DescriptorAllocator& desc_alloc);

    /// destroys all elements inside, and clears the map
    void reset(VkDevice device, DescriptorAllocator& desc_alloc);

private:
    static size_t hashKey(key_t const& v)
    {
        size_t res = 0;
        for (auto const& elem : v.reflected_ranges)
        {
            auto const elem_hash = hash::detail::hash_combine(hash::detail::hash(elem.set, elem.type, elem.binding_size, elem.binding_start),
                                                              hash::detail::hash(elem.visible_stages));
            res = hash::detail::hash_combine(res, elem_hash);
        }
        return hash::detail::hash_combine(res, hash::compute(v.arg_samplers));
    }

    backend::detail::cache_map<pipeline_layout> mCache;
};

}
