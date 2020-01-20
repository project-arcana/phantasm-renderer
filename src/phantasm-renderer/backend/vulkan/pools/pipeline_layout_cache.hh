#pragma once

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/cache_map.hh>

#include <phantasm-renderer/backend/vulkan/loader/vulkan_fwd.hh>
#include <phantasm-renderer/backend/vulkan/pipeline_layout.hh>

namespace pr::backend::vk
{
class DescriptorAllocator;

/// Persistent cache for pipeline layouts
/// Unsynchronized, only used inside of pipeline pool
class PipelineLayoutCache
{
public:
    void initialize(unsigned max_elements);
    void destroy(VkDevice device);

    /// receive an existing root signature matching the shape, or create a new one
    /// returns a pointer which remains stable
    [[nodiscard]] pipeline_layout* getOrCreate(VkDevice device, cc::span<util::spirv_desc_info const> reflected_ranges, bool has_push_constants);

    /// destroys all elements inside, and clears the map
    void reset(VkDevice device);

private:
    static size_t hashKey(cc::span<util::spirv_desc_info const> reflected_ranges, bool has_push_constants);

    backend::detail::cache_map<pipeline_layout> mCache;
};

}
