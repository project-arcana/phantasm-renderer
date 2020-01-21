#pragma once

#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/gpu_info.hh>
#include <phantasm-renderer/backend/types.hh>

#include "common/d3d12_sanitized.hh"

namespace pr::backend::d3d12
{
[[nodiscard]] gpu_feature_flags check_capabilities(ID3D12Device* device);

/// Test the given adapter by creating a device with the min_feature level, returns the amount of device nodes, >= 0 on success
[[nodiscard]] int test_adapter(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL min_features, D3D_FEATURE_LEVEL& out_max_features);

/// Get all available adapter candidates
[[nodiscard]] cc::vector<gpu_info> get_adapter_candidates();
}
