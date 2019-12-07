#pragma once

#include <mutex>

#include <clean-core/span.hh>
#include <clean-core/capped_vector.hh>
#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/arguments.hh>

#include <phantasm-renderer/backend/vulkan/resources/descriptor_allocator.hh>

namespace pr::backend::vk
{
class ResourcePool;

/// The high-level allocator for shader views
/// Synchronized
class ShaderViewPool
{
public:
    // frontend-facing API

    [[nodiscard]] handle::shader_view create(cc::span<shader_view_element const> srvs, cc::span<shader_view_element const> uavs);

    void free(handle::shader_view sv);

public:
    // internal API
    void initialize(VkDevice device, ResourcePool* res_pool, int num_cbvs, int num_srvs, int num_uavs, int num_samplers);
    void destroy();

    [[nodiscard]] VkDescriptorSet get(handle::shader_view sv) const { return mPool.get(static_cast<unsigned>(sv.index)).raw_desc_set; }
    [[nodiscard]] cc::span<handle::resource const> getResources(handle::shader_view sv) const {
        return mPool.get(static_cast<unsigned>(sv.index)).resources;
    }

    [[nodiscard]] VkImageView makeImageView(shader_view_element const& sve) const;

private:
    struct shader_view_node
    {
        VkDescriptorSet raw_desc_set;
        // handles contained in this shader view, for state tracking
        cc::capped_vector<handle::resource, 16> resources;
        // image views in use by this shader view
        cc::vector<VkImageView> image_views;
    };

    // non-owning
    VkDevice mDevice;
    ResourcePool* mResourcePool;

    /// The main pool data
    backend::detail::linked_pool<shader_view_node, unsigned> mPool;

    /// "Backing" allocator
    DescriptorAllocator mAllocator;
    std::mutex mMutex;
};

}
