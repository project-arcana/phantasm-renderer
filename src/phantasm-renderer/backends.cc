#include "backends.hh"

#ifdef PHI_BACKEND_VULKAN
#include <phantasm-hardware-interface/vulkan/BackendVulkan.hh>
#endif

#ifdef PHI_BACKEND_D3D12
#include <phantasm-hardware-interface/d3d12/BackendD3D12.hh>
#endif

phi::Backend* pr::make_vulkan_backend(phi::window_handle const& window_handle, const phi::backend_config& cfg)
{
#ifdef PHI_BACKEND_VULKAN
    phi::Backend* const res = new phi::vk::BackendVulkan();
    res->initialize(cfg, window_handle);
    return res;
#else
    CC_RUNTIME_ASSERT(false && "vulkan backend disabled");
    return nullptr;
#endif
}

phi::Backend* pr::make_d3d12_backend(phi::window_handle const& window_handle, const phi::backend_config& cfg)
{
#ifdef PHI_BACKEND_D3D12
    phi::Backend* const res = new phi::d3d12::BackendD3D12();
    res->initialize(cfg, window_handle);
    return res;
#else
    CC_RUNTIME_ASSERT(false && "d3d12 backend disabled");
    return nullptr;
#endif
}
