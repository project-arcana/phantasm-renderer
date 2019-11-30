#pragma once

#include <unordered_map>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/hash.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#include <phantasm-renderer/backend/vulkan/shader_arguments.hh>

namespace pr::backend::vk
{
/// Persistent cache for pipeline layouts
/// Unsynchronized, only used inside of pipeline pool
class PipelineLayoutCache
{
public:
    void initialize(unsigned size_estimate = 50);
    void destroy(VkDevice device);

    /// receive an existing root signature matching the shape, or create a new one
    /// returns a pointer (which remains stable, §23.2.5/13 C++11)
    [[nodiscard]] pipeline_layout* getOrCreate(VkDevice device, cc::span<util::spirv_desc_range_info const> range_infos);

    /// destroys all elements inside, and clears the map
    void reset(VkDevice device);

private:
    using key_t = cc::span<util::spirv_desc_range_info const>;

    struct key_hasher_functor
    {
        std::size_t operator()(key_t const& v) const noexcept
        {
            size_t res = 0;
            for (auto const& elem : v)
            {
                auto const elem_hash = hash::detail::hash_combine(hash::detail::hash(elem.set, elem.type, elem.binding_size, elem.binding_start),
                                                                  hash::detail::hash(elem.visible_stage));
                res = hash::detail::hash_combine(res, elem_hash);
            }
            return res;
        }
    };

    struct key_equal_op
    {
        bool operator()(key_t const& lhs, key_t const& rhs) const noexcept
        {
            if (lhs.size() != rhs.size())
                return false;

            for (auto i = 0u; i < lhs.size(); ++i)
            {
                if (!(lhs[i] == rhs[i]))
                    return false;
            }

            return true;
        }
    };

    std::unordered_map<key_t, pipeline_layout, key_hasher_functor, key_equal_op> mCache;
};

}
