#include "resource_pool.hh"

#include <phantasm-renderer/backend/d3d12/common/d3dx12.hh>
#include <phantasm-renderer/backend/d3d12/common/dxgi_format.hh>
#include <phantasm-renderer/backend/d3d12/memory/D3D12MA.hh>

#include "resource_state.hh"

void pr::backend::d3d12::ResourcePool::initialize(ID3D12Device& device, unsigned max_num_resources)
{
    mAllocator.initialize(device);
    // TODO: think about reserved bits for dangle check, assert max_num < 2^free bits
    mPool.initialize(max_num_resources);
}

pr::backend::handle::resource pr::backend::d3d12::ResourcePool::createTexture2D(backend::format format, int w, int h, int mips)
{
    constexpr auto initial_state = resource_state::copy_dest;

    // NOTE: flags
    auto const desc = CD3DX12_RESOURCE_DESC::Tex2D(util::to_dxgi_format(format), UINT(w), UINT(h), 1, UINT16(mips), 1, 0, D3D12_RESOURCE_FLAG_NONE);

    auto* const alloc = mAllocator.allocateResourceRaw(desc, to_resource_states(initial_state));
    return acquireResource(alloc, initial_state);
}

pr::backend::handle::resource pr::backend::d3d12::ResourcePool::createRenderTarget(pr::backend::format format, int w, int h)
{
    auto const format_dxgi = util::to_dxgi_format(format);
    if (is_depth_format(format))
    {
        // Depth-stencil target
        constexpr auto initial_state = resource_state::depth_write;

        D3D12_CLEAR_VALUE clear_value;
        clear_value.Format = format_dxgi;
        clear_value.DepthStencil.Depth = 1;
        clear_value.DepthStencil.Stencil = 0;

        auto const desc = CD3DX12_RESOURCE_DESC::Tex2D(format_dxgi, UINT(w), UINT(h), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        auto* const alloc = mAllocator.allocateResourceRaw(desc, to_resource_states(initial_state), &clear_value);
        return acquireResource(alloc, initial_state);
    }
    else
    {
        // Render target
        constexpr auto initial_state = resource_state::render_target;

        D3D12_CLEAR_VALUE clear_value;
        clear_value.Format = format_dxgi;
        clear_value.Color[0] = 0.0f;
        clear_value.Color[1] = 0.0f;
        clear_value.Color[2] = 0.0f;
        clear_value.Color[3] = 1.0f;

        auto const desc = CD3DX12_RESOURCE_DESC::Tex2D(format_dxgi, UINT(w), UINT(h), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

        auto* const alloc = mAllocator.allocateResourceRaw(desc, to_resource_states(initial_state), &clear_value);
        return acquireResource(alloc, initial_state);
    }
}

pr::backend::handle::resource pr::backend::d3d12::ResourcePool::createBuffer(unsigned size_bytes, pr::backend::resource_state initial_state, unsigned stride_bytes)
{
    auto const desc = CD3DX12_RESOURCE_DESC::Buffer(size_bytes);
    auto* const alloc = mAllocator.allocateResourceRaw(desc, to_resource_states(initial_state));
    return acquireResource(alloc, initial_state, size_bytes, stride_bytes);
}

void pr::backend::d3d12::ResourcePool::freeResource(pr::backend::handle::resource res)
{
    // TODO: dangle check
    // TODO: Do we internally keep the resource alive until it is no longer used, or
    // do we require this to only happen after that point?

    // This requires no synchronization, as D3D12MA internally syncs
    resource_node& freed_node = mPool.get(static_cast<unsigned>(res.index));
    freed_node.allocation->Release();

    {
        // This is a write access to the pool and must be synced
        auto lg = std::lock_guard(mMutex);
        mPool.release(static_cast<unsigned>(res.index));
    }
}

pr::backend::handle::resource pr::backend::d3d12::ResourcePool::acquireResource(D3D12MA::Allocation* alloc,
                                                                                pr::backend::resource_state initial_state,
                                                                                unsigned buffer_width,
                                                                                unsigned buffer_stride)
{
    unsigned res;
    {
        // This is a write access to the pool and must be synced
        auto lg = std::lock_guard(mMutex);
        res = mPool.acquire();
    }
    resource_node& new_node = mPool.get(res);
    new_node.allocation = alloc;
    new_node.resource = alloc->GetResource();

    new_node.master_state = initial_state;
    new_node.buffer_width = buffer_width;
    new_node.buffer_stride = buffer_stride;

    return {static_cast<handle::index_t>(res)};
}
