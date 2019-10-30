#pragma once
#ifdef PR_BACKEND_D3D12

#include <clean-core/typedefs.hh>

#include "common/d3d12_sanitized.hh"

namespace pr::backend::d3d12
{
enum class adapter_preference : char
{
    highest_vram,
    first,
    integrated,
    highest_feature_level,
    explicit_index
};

/**
 * Configuration for creating a D3D12 backend
 */
struct d3d12_config
{
    bool enable_validation = false;     ///< Whether to enable debug layers, requires installed D3D12 SDK
    bool enable_gpu_validation = false; ///< Whether to enable GPU Validation, can cause slowdown, requires enable_validation == true

    bool enable_dxr = true; ///< Enable DXR features if available

    adapter_preference adapter_preference = adapter_preference::highest_vram;
    cc::uint32 explicit_adapter_index = cc::uint32(-1);

    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_12_0; ///< The feature level to query in ::D3D12CreateDevice
};
}

#endif
