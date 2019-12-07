#pragma once

#include <mutex>

#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/vulkan/memory/ResourceAllocator.hh>
#include <phantasm-renderer/backend/vulkan/resources/descriptor_allocator.hh>

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
    [[nodiscard]] std::byte* getMappedMemory(handle::resource res) { return mPool.get(static_cast<unsigned>(res.index)).buffer.map; }

public:
    struct resource_node
    {
    public:
        enum class resource_type : uint8_t
        {
            buffer,
            image
        };

        struct buffer_info
        {
            VkBuffer raw_buffer;
            /// a descriptor set containing a single UNIFORM_BUFFER_DYNAMIC descriptor,
            /// unconditionally created for all buffers
            VkDescriptorSet raw_uniform_dynamic_ds;
            uint32_t width;
            uint32_t stride; ///< vertex size or index size
            std::byte* map;
        };

        struct image_info
        {
            VkImage raw_image;
            format pixel_format;
            unsigned num_mips;
            unsigned num_array_layers;
        };

    public:
        VmaAllocation allocation;

        union {
            buffer_info buffer;
            image_info image;
        };

        VkPipelineStageFlags master_state_dependency;
        resource_state master_state;
        resource_type type;
    };

public:
    // internal API

    void initialize(VkPhysicalDevice physical, VkDevice device, unsigned max_num_resources);
    void destroy();

    //
    // Raw VkBuffer / VkImage access
    //

    [[nodiscard]] VkBuffer getRawBuffer(handle::resource res) const { return internalGet(res).buffer.raw_buffer; }
    [[nodiscard]] VkImage getRawImage(handle::resource res) const { return internalGet(res).image.raw_image; }

    // Raw CBV uniform buffer dynamic descriptor set access
    [[nodiscard]] VkDescriptorSet getRawCBVDescriptorSet(handle::resource res) const { return internalGet(res).buffer.raw_uniform_dynamic_ds; }

    // Additional information
    [[nodiscard]] bool isImage(handle::resource res) const { return internalGet(res).type == resource_node::resource_type::image; }
    [[nodiscard]] resource_node::image_info const& getImageInfo(handle::resource res) const { return internalGet(res).image; }
    [[nodiscard]] resource_node::buffer_info const& getBufferInfo(handle::resource res) const { return internalGet(res).buffer; }

    //
    // Master state access
    //

    [[nodiscard]] resource_state getResourceState(handle::resource res) const { return mPool.get(static_cast<unsigned>(res.index)).master_state; }
    [[nodiscard]] VkPipelineStageFlags getResourceStageDependency(handle::resource res) const
    {
        return mPool.get(static_cast<unsigned>(res.index)).master_state_dependency;
    }

    void setResourceState(handle::resource res, resource_state new_state, VkPipelineStageFlags new_state_dep)
    {
        // This is a write access to the pool, however we require
        // no sync since it would not interfere with unrelated allocs and frees
        // and this call assumes exclusive access to the given resource
        auto& node = internalGet(res);
        node.master_state = new_state;
        node.master_state_dependency = new_state_dep;
    }

    //
    // Swapchain backbuffer resource injection
    // Swapchain backbuffers are exposed as handle::resource, so they can be interchangably
    // used with any other render target, and follow the same transition semantics
    // these handles have a limited lifetime: valid from Backend::acquireBackbuffer until
    // the first of either Backend::present or Backend::resize
    //

    [[nodiscard]] handle::resource injectBackbufferResource(VkImage raw_image, resource_state state, VkImageView backbuffer_view);

    [[nodiscard]] bool isBackbuffer(handle::resource res) const { return res == mInjectedBackbufferResource; }

    [[nodiscard]] VkImageView getBackbufferView() const { return mInjectedBackbufferView; }

private:
    [[nodiscard]] handle::resource acquireBuffer(
        VmaAllocation alloc, VkBuffer buffer, resource_state initial_state, unsigned buffer_width = 0, unsigned buffer_stride = 0, std::byte* buffer_map = nullptr);

    [[nodiscard]] handle::resource acquireImage(VmaAllocation alloc, VkImage buffer, format pixel_format, resource_state initial_state, unsigned num_mips, unsigned num_array_layers);

    [[nodiscard]] resource_node const& internalGet(handle::resource res) const { return mPool.get(static_cast<unsigned>(res.index)); }
    [[nodiscard]] resource_node& internalGet(handle::resource res) { return mPool.get(static_cast<unsigned>(res.index)); }

    void internalFree(resource_node& node);

private:
    /// The main pool data
    backend::detail::linked_pool<resource_node, unsigned> mPool;

    /// The handle of the injected backbuffer resource
    handle::resource mInjectedBackbufferResource = handle::null_resource;

    /// The image view of the currently injected backbuffer, stored separately to
    /// not take up space in resource_node, there is always just a single injected backbuffer
    VkImageView mInjectedBackbufferView = nullptr;

    /// "Backing" allocators
    ResourceAllocator mAllocator;
    DescriptorAllocator mAllocatorDescriptors;
    std::mutex mMutex;
};

}
