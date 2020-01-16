#pragma once

#include <mutex>

#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/d3d12/memory/ResourceAllocator.hh>

namespace pr::backend::d3d12
{
/// The high-level allocator for resources
/// Synchronized
/// Exception: ::setResourceState (see master state cache)
class ResourcePool
{
public:
    // frontend-facing API

    /// create a 1D, 2D or 3D texture, or a 1D/2D array
    [[nodiscard]] handle::resource createTexture(
        format format, unsigned w, unsigned h, unsigned mips, texture_dimension dim = texture_dimension::t2d, unsigned depth_or_array_size = 1, bool allow_uav = false);

    /// create a render- or depth-stencil target
    [[nodiscard]] handle::resource createRenderTarget(backend::format format, unsigned w, unsigned h, unsigned samples);

    /// create a buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createBuffer(unsigned size_bytes, unsigned stride_bytes, bool allow_uav);

    [[nodiscard]] handle::resource createBufferInternal(unsigned size_bytes, unsigned stride_bytes, bool allow_uav, D3D12_RESOURCE_STATES initial_state);

    /// create a mapped, UPLOAD_HEAP buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createMappedBuffer(unsigned size_bytes, unsigned stride_bytes = 0);

    void free(handle::resource res);
    void free(cc::span<handle::resource const> resources);

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
            uint32_t width;
            uint32_t stride; ///< vertex size or index size
            std::byte* map;
        };

        struct image_info
        {
            format pixel_format;
            unsigned num_mips;
            unsigned num_array_layers;
        };

    public:
        D3D12MA::Allocation* allocation = nullptr;
        ID3D12Resource* resource = nullptr;

        union {
            buffer_info buffer;
            image_info image;
        };

        resource_state master_state = resource_state::unknown;
        resource_type type;
    };

public:
    // internal API

    void initialize(ID3D12Device& device, unsigned max_num_resources);
    void destroy();

    //
    // Raw ID3D12Resource access
    //

    [[nodiscard]] ID3D12Resource* getRawResource(handle::resource res) const { return internalGet(res).resource; }

    // Additional information
    [[nodiscard]] bool isImage(handle::resource res) const { return internalGet(res).type == resource_node::resource_type::image; }
    [[nodiscard]] resource_node::image_info const& getImageInfo(handle::resource res) const { return internalGet(res).image; }
    [[nodiscard]] resource_node::buffer_info const& getBufferInfo(handle::resource res) const { return internalGet(res).buffer; }

    //
    // Master state access
    //

    [[nodiscard]] resource_state getResourceState(handle::resource res) const { return internalGet(res).master_state; }

    void setResourceState(handle::resource res, resource_state new_state)
    {
        // This is a write access to the pool, however we require
        // no sync since it would not interfere with unrelated allocs and frees
        // and this call assumes exclusive access to the given resource
        internalGet(res).master_state = new_state;
    }

    //
    // CPU buffer view creation
    //

    [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(handle::resource res) const
    {
        auto const& data = internalGet(res);
        CC_ASSERT(data.type == resource_node::resource_type::buffer);
        return {data.resource->GetGPUVirtualAddress(), data.buffer.width, data.buffer.stride};
    }

    [[nodiscard]] D3D12_INDEX_BUFFER_VIEW getIndexBufferView(handle::resource res) const
    {
        auto const& data = internalGet(res);
        CC_ASSERT(data.type == resource_node::resource_type::buffer);
        return {data.resource->GetGPUVirtualAddress(), data.buffer.width, (data.buffer.stride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT};
    }

    [[nodiscard]] D3D12_CONSTANT_BUFFER_VIEW_DESC getConstantBufferView(handle::resource res) const
    {
        auto const& data = internalGet(res);
        CC_ASSERT(data.type == resource_node::resource_type::buffer);
        return {data.resource->GetGPUVirtualAddress(), data.buffer.width};
    }

    //
    // Swapchain backbuffer resource injection
    // Swapchain backbuffers are exposed as handle::resource, so they can be interchangably
    // used with any other render target, and follow the same transition semantics
    // these handles have a limited lifetime: valid from Backend::acquireBackbuffer until
    // the first of either Backend::present or Backend::resize
    //

    [[nodiscard]] handle::resource injectBackbufferResource(ID3D12Resource* raw_resource, resource_state state);

    [[nodiscard]] bool isBackbuffer(handle::resource res) const { return res == mInjectedBackbufferResource; }

private:
    [[nodiscard]] handle::resource acquireBuffer(
        D3D12MA::Allocation* alloc, resource_state initial_state, unsigned buffer_width = 0, unsigned buffer_stride = 0, std::byte* buffer_map = nullptr);

    [[nodiscard]] handle::resource acquireImage(D3D12MA::Allocation* alloc, format pixel_format, resource_state initial_state, unsigned num_mips, unsigned num_array_layers);

    [[nodiscard]] resource_node const& internalGet(handle::resource res) const { return mPool.get(static_cast<unsigned>(res.index)); }
    [[nodiscard]] resource_node& internalGet(handle::resource res) { return mPool.get(static_cast<unsigned>(res.index)); }

private:
    /// The main pool data
    backend::detail::linked_pool<resource_node, unsigned> mPool;

    handle::resource mInjectedBackbufferResource = handle::null_resource;

    /// "Backing" allocator
    ResourceAllocator mAllocator;
    std::mutex mMutex;
};

}
