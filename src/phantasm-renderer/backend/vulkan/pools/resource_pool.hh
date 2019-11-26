#pragma once

#include <mutex>

#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/memory/ResourceAllocator.hh>

namespace pr::backend::vk
{
/// The high-level allocator for resources
/// Synchronized
/// Exception: ::setResourceState (see master state cache)
class ResourcePool
{
public:
    // frontend-facing API

    /// create a 2D texture
    [[nodiscard]] handle::resource createTexture2D(backend::format format, int w, int h, int mips);

    /// create a render- or depth-stencil target
    [[nodiscard]] handle::resource createRenderTarget(backend::format format, int w, int h);

    /// create a buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createBuffer(unsigned size_bytes, resource_state initial_state, unsigned stride_bytes = 0);

    /// create a mapped, UPLOAD_HEAP buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createMappedBuffer(unsigned size_bytes, unsigned stride_bytes = 0);

    void free(handle::resource res);

    /// only valid for resources created with createMappedBuffer
    [[nodiscard]] std::byte* getMappedMemory(handle::resource res) { return mPool.get(static_cast<unsigned>(res.index)).buffer_map; }

public:
    // internal API

    void initialize(VkPhysicalDevice physical, VkDevice device, unsigned max_num_resources);
    void destroy();

    //
    // Raw VkBuffer / VkImage access
    //

    [[nodiscard]] VkBuffer getRawBuffer(handle::resource res) const { return mPool.get(static_cast<unsigned>(res.index)).buffer; }
    [[nodiscard]] VkImage getRawImage(handle::resource res) const { return mPool.get(static_cast<unsigned>(res.index)).image; }

    //
    // Master state access
    //

    [[nodiscard]] resource_state getResourceState(handle::resource res) const { return mPool.get(static_cast<unsigned>(res.index)).master_state; }

    void setResourceState(handle::resource res, resource_state new_state)
    {
        // This is a write access to the pool, however we require
        // no sync since it would not interfere with unrelated allocs and frees
        // and this call assumes exclusive access to the given resource
        mPool.get(static_cast<unsigned>(res.index)).master_state = new_state;
    }

    //
    // Swapchain backbuffer resource injection
    // Swapchain backbuffers are exposed as handle::resource, so they can be interchangably
    // used with any other render target, and follow the same transition semantics
    // these handles have a limited lifetime: valid from Backend::acquireBackbuffer until
    // the first of either Backend::present or Backend::resize
    //

    [[nodiscard]] handle::resource injectBackbufferResource(VkImage raw_image, resource_state state);

private:
    struct resource_node
    {
        VmaAllocation allocation;

        union {
            VkBuffer buffer;
            VkImage image;
        };

        // additional information for (CPU) buffer views,
        // only valid if the resource was created with ::createBuffer
        uint32_t buffer_width;
        uint32_t buffer_stride; ///< vertex size or index size
        std::byte* buffer_map;

        resource_state master_state;

        enum class resource_type : uint8_t
        {
            buffer,
            image
        };
        resource_type type;
    };
private:
    [[nodiscard]] handle::resource acquireBuffer(
        VmaAllocation alloc, VkBuffer buffer, resource_state initial_state, unsigned buffer_width = 0, unsigned buffer_stride = 0, std::byte* buffer_map = nullptr);

    [[nodiscard]] handle::resource acquireImage(VmaAllocation alloc, VkImage buffer, resource_state initial_state);

    void internalFree(resource_node& node);

private:

    /// The main pool data
    backend::detail::linked_pool<resource_node, unsigned> mPool;

    handle::resource mInjectedBackbufferResource = handle::null_resource;

    /// "Backing" allocator
    ResourceAllocator mAllocator;
    std::mutex mMutex;
};

}
