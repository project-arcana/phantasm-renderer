#pragma once

#include <string>

#include <clean-core/vector.hh>
#include <clean-core/span.hh>

#include "common/d3d12_sanitized.hh"

#include "d3d12_config.hh"

namespace pr::backend::d3d12
{
enum class adapter_vendor
{
    amd,
    intel,
    nvidia,
    unknown
};

struct adapter_candidate
{
    adapter_vendor vendor;
    unsigned index;

    size_t dedicated_video_memory_bytes;
    size_t dedicated_system_memory_bytes;
    size_t shared_system_memory_bytes;

    std::string description;
    D3D_FEATURE_LEVEL max_feature_level;
};

/// Converts a vendor ID as received from a DXGI_ADAPTER_DESC to the adapter_vendor enum
[[nodiscard]] adapter_vendor get_vendor_from_id(unsigned id);

/// Test the given adapter by creating a device with the min_feature level, returns the amount of device nodes, >= 0 on success
[[nodiscard]] int test_adapter(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL min_features, D3D_FEATURE_LEVEL& out_max_features);

/// Get all available adapter candidates
[[nodiscard]] cc::vector<adapter_candidate> get_adapter_candidates();

[[nodiscard]] unsigned get_preferred_adapter_index(cc::span<adapter_candidate const> candidates, adapter_preference preference);
}
