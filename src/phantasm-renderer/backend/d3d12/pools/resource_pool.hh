#pragma once

#include <mutex>

#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/d3d12/resources/resource.hh>

namespace pr::backend::d3d12
{
/// The high-level allocator for resources
/// Synchronized
/// Exception: ::setResourceState (see master state cache)
class ResourcePool
{
public:
    // frontend-facing API

    /// create a 2D texture2
    [[nodiscard]] handle::resource createTexture2D(backend::format format, int w, int h, int mips);

    /// create a render- or depth-stencil target
    [[nodiscard]] handle::resource createRenderTarget(backend::format format, int w, int h);

    /// create a buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createBuffer(unsigned size_bytes, resource_state initial_state, unsigned stride_bytes = 0);

    void free(handle::resource res);

public:
    // internal API

    void initialize(ID3D12Device& device, unsigned max_num_resources);

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

private:
    [[nodiscard]] handle::resource acquireResource(D3D12MA::Allocation* alloc, resource_state initial_state, unsigned buffer_width = 0, unsigned buffer_stride = 0);

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
    };

    /// The main pool data
    backend::detail::linked_pool<resource_node, unsigned> mPool;

    /// "Backing" allocator
    ResourceAllocator mAllocator;
    std::mutex mMutex;
};

}