#include "backends.hh"

#include <clean-core/poly_unique_ptr.hh>

#ifdef PR_BACKEND_VULKAN
#include <phantasm-renderer/backend/vulkan/BackendVulkan.hh>
#endif

#ifdef PR_BACKEND_D3D12
#include <phantasm-renderer/backend/d3d12/BackendD3D12.hh>
#endif

cc::poly_unique_ptr<pr::backend::Backend> pr::make_vulkan_backend(backend::window_handle const& window_handle, const pr::backend::backend_config& cfg)
{
#ifdef PR_BACKEND_VULKAN
    auto res = cc::make_poly_unique<backend::vk::BackendVulkan>();
    res->initialize(cfg, window_handle);
    return std::move(res);
#else
    CC_RUNTIME_ASSERT(false && "vulkan backend disabled");
    return {};
#endif
}

cc::poly_unique_ptr<pr::backend::Backend> pr::make_d3d12_backend(backend::window_handle const& window_handle, const pr::backend::backend_config& cfg)
{
#ifdef PR_BACKEND_D3D12
    auto res = cc::make_poly_unique<backend::d3d12::BackendD3D12>();
    res->initialize(cfg, window_handle);
    return std::move(res);
#else
    CC_RUNTIME_ASSERT(false && "d3d12 backend disabled");
    return {};
#endif
}
