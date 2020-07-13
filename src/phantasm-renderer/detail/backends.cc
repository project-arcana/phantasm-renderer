#include "backends.hh"

#include <clean-core/assert.hh>

#include <phantasm-hardware-interface/Backend.hh>

#ifdef PHI_BACKEND_VULKAN
#include <phantasm-hardware-interface/vulkan/BackendVulkan.hh>
#endif

#ifdef PHI_BACKEND_D3D12
#include <phantasm-hardware-interface/d3d12/BackendD3D12.hh>
#endif

phi::Backend* pr::detail::make_backend(backend type)
{
    phi::Backend* res = nullptr;
    if (type == backend::d3d12)
    {
#ifdef PHI_BACKEND_D3D12
        res = new phi::d3d12::BackendD3D12();
#else
        CC_RUNTIME_ASSERT(false && "d3d12 backend disabled");
#endif
    }
    else if (type == backend::vulkan)
    {
#ifdef PHI_BACKEND_VULKAN
        res = new phi::vk::BackendVulkan();
#else
        CC_RUNTIME_ASSERT(false && "vulkan backend disabled");
#endif
    }
    else
    {
        CC_RUNTIME_ASSERT(false && "unknown backend requested");
    }

    return res;
}
