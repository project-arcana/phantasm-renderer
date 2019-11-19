#pragma once

#include <mutex>

#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/types.hh>

#include "resource.hh"

namespace pr::backend::d3d12
{
/// Synchronized
/// Exception: ::setResourceState (see master state cache)
class ResourcePool
{
public:
    void initialize(ID3D12Device& device, unsigned max_num_resources);

public:
    // Backend-facing API

    [[nodiscard]] handle::resource createTexture2D(backend::format format, int w, int h, int mips);
    [[nodiscard]] handle::resource createRenderTarget(backend::format format, int w, int h);
    [[nodiscard]] handle::resource createBuffer(size_t size_bytes, resource_state initial_state);

    void freeResource(handle::resource res);

public:
    // Internal API

    [[nodiscard]] ID3D12Resource* getRawResource(handle::resource res) const;

    [[nodiscard]] resource_state getResourceState(handle::resource res) const;
    void setResourceState(handle::resource res, resource_state new_state);

private:
    [[nodiscard]] handle::resource acquireResource(D3D12MA::Allocation* alloc, resource_state initial_state);

private:
    struct resource_node
    {
        D3D12MA::Allocation* allocation = nullptr;
        resource_state master_state = resource_state::unknown;
    };

    backend::detail::linked_pool<resource_node, unsigned> mPool;
    ResourceAllocator mAllocator;
    std::mutex mMutex;
};

}
