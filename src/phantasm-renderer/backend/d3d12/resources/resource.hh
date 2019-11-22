#pragma once

#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>

namespace D3D12MA
{
class Allocator;
class Allocation;
}

namespace pr::backend::d3d12
{
class ResourceAllocator;

// move-only
struct resource
{
    resource() = default;
    resource(resource const&) = delete;
    resource& operator=(resource const&) = delete;
    ~resource();

    resource(resource&& rhs) noexcept
    {
        raw = rhs.raw;
        _allocation = rhs._allocation;
        // invalidate rhs
        rhs._allocation = nullptr;
    }

    resource& operator=(resource&& rhs) noexcept
    {
        if (this != &rhs)
        {
            // possibly free previous allocation
            free();
            raw = rhs.raw;
            _allocation = rhs._allocation;
            // invalidate rhs
            rhs._allocation = nullptr;
        }

        return *this;
    }

    ID3D12Resource* raw;

    D3D12MA::Allocation* get_allocation() const { return _allocation; }

private:
    friend class ResourceAllocator;
    D3D12MA::Allocation* _allocation = nullptr;

    /// free this resource, thread safe
    /// only use this method, do not call ->Release() on any member
    void free();
};

class ResourceAllocator
{
public:
    ResourceAllocator() = default;
    ResourceAllocator(ResourceAllocator const&) = delete;
    ResourceAllocator(ResourceAllocator&&) noexcept = delete;
    ResourceAllocator& operator=(ResourceAllocator const&) = delete;
    ResourceAllocator& operator=(ResourceAllocator&&) noexcept = delete;

public:
    void initialize(ID3D12Device& device);
    void destroy();

    /// allocate a resource, thread safe
    [[nodiscard]] resource allocateResource(D3D12_RESOURCE_DESC const& desc,
                                            D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON,
                                            D3D12_CLEAR_VALUE* clear_value = nullptr,
                                            D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT) const;

    [[nodiscard]] D3D12MA::Allocation* allocateResourceRaw(D3D12_RESOURCE_DESC const& desc,
                                                           D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON,
                                                           D3D12_CLEAR_VALUE* clear_value = nullptr,
                                                           D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT) const;

private:
    D3D12MA::Allocator* mAllocator = nullptr;
};
}
