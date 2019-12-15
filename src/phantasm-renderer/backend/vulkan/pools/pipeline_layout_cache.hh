#pragma once

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/cache_map.hh>
#include <phantasm-renderer/backend/detail/hash.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#include <phantasm-renderer/backend/vulkan/pipeline_layout.hh>

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
        cc::span<util::spirv_desc_info const> reflected_ranges;
    };

    void initialize(unsigned max_elements);
    void destroy(VkDevice device);

    /// receive an existing root signature matching the shape, or create a new one
    /// returns a pointer (which remains stable, §23.2.5/13 C++11)
    [[nodiscard]] pipeline_layout* getOrCreate(VkDevice device, cc::span<util::spirv_desc_info const> reflected_ranges, bool has_push_constants);

    /// destroys all elements inside, and clears the map
    void reset(VkDevice device);

private:
    static size_t hashKey(cc::span<util::spirv_desc_info const> reflected_ranges, bool has_push_constants)
    {
        size_t res = cc::make_hash(has_push_constants);
        for (auto const& elem : reflected_ranges)
        {
            auto const elem_hash = cc::make_hash(elem.set, elem.type, elem.binding, elem.binding_array_size, elem.visible_stage);
            res = cc::hash_combine(res, elem_hash);
        }
        return res;
    }

    backend::detail::cache_map<pipeline_layout> mCache;
};

}
