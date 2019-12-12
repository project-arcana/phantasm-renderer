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
    [[nodiscard]] handle::resource createTexture(format format, unsigned w, unsigned h, unsigned mips, texture_dimension dim = texture_dimension::t2d, unsigned depth_or_array_size = 1);

    /// create a render- or depth-stencil target
    [[nodiscard]] handle::resource createRenderTarget(backend::format format, unsigned w, unsigned h, unsigned samples);

    /// create a buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createBuffer(unsigned size_bytes, resource_state initial_state, unsigned stride_bytes = 0);

    /// create a mapped, UPLOAD_HEAP buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createMappedBuffer(unsigned size_bytes, unsigned stride_bytes = 0);

    void free(handle::resource res);
    void free(cc::span<handle::resource const> resources);

    /// only valid for resources created with createMappedBuffer
    [[nodiscard]] std::byte* getMappedMemory(handle::resource res) { return mPool.get(static_cast<unsigned>(res.index)).buffer_map; }

public:
    // internal API

    void initialize(ID3D12Device& device, unsigned max_num_resources);
    void destroy();

    //
    // Raw ID3D12Resource access
    //

    [[nodiscard]] ID3D12Resource* getRawResource(handle::resource res) const { return mPool.get(static_cast<unsigned>(res.index)).resource; }

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
    // CPU buffer view creation
    //

    [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW getVertexBufferView(handle::resource res) const
    {
        auto const& data = mPool.get(static_cast<unsigned>(res.index));
        return {data.resource->GetGPUVirtualAddress(), data.buffer_width, data.buffer_stride};
    }

    [[nodiscard]] D3D12_INDEX_BUFFER_VIEW getIndexBufferView(handle::resource res) const
    {
        auto const& data = mPool.get(static_cast<unsigned>(res.index));
        return {data.resource->GetGPUVirtualAddress(), data.buffer_width, (data.buffer_stride == 4) ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT};
    }

    [[nodiscard]] D3D12_CONSTANT_BUFFER_VIEW_DESC getConstantBufferView(handle::resource res) const
    {
        auto const& data = mPool.get(static_cast<unsigned>(res.index));
        return {data.resource->GetGPUVirtualAddress(), data.buffer_width};
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
    [[nodiscard]] handle::resource acquireResource(
        D3D12MA::Allocation* alloc, resource_state initial_state, unsigned buffer_width = 0, unsigned buffer_stride = 0, std::byte* buffer_map = nullptr);

private:
    struct resource_node
    {
        D3D12MA::Allocation* allocation = nullptr;
        ID3D12Resource* resource = nullptr;

        resource_state master_state = resource_state::unknown;

        // additional information for (CPU) buffer views,
        // only valid if the resource was created with ::createBuffer
        UINT buffer_width;
        UINT buffer_stride; ///< vertex size or index size
        std::byte* buffer_map;
    };

    /// The main pool data
    backend::detail::linked_pool<resource_node, unsigned> mPool;

    handle::resource mInjectedBackbufferResource = handle::null_resource;

    /// "Backing" allocator
    ResourceAllocator mAllocator;
    std::mutex mMutex;
};

}
