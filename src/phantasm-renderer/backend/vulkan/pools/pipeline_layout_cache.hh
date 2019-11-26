#pragma once

#include <unordered_map>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/hash.hh>

#include <phantasm-renderer/backend/vulkan/loader/volk.hh>
#include <phantasm-renderer/backend/vulkan/shader_arguments.hh>

namespace pr::backend::vk
{
/// Persistent cache for root signatures
/// Unsynchronized, only used inside of pso pool
class RootSignatureCache
{
public:
    void initialize(unsigned size_estimate = 50);
    void destroy(VkDevice device);

    /// receive an existing root signature matching the shape, or create a new one
    /// returns a pointer (which remains stable, §23.2.5/13 C++11)
    [[nodiscard]] pipeline_layout* getOrCreate(VkDevice device, arg::shader_argument_shapes arg_shapes);

    /// destroys all elements inside, and clears the map
    void reset(VkDevice device);

private:
    using key_t = arg::shader_argument_shapes;
    std::unordered_map<key_t, pipeline_layout, hash::compute_functor> mCache;
};

}
