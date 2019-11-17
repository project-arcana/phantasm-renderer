#include "Device.hh"

#include <iostream>

#include "common/verify.hh"

void pr::backend::d3d12::Device::initialize(IDXGIAdapter& adapter, const pr::backend::d3d12::d3d12_config& config)
{
    PR_D3D12_VERIFY(::D3D12CreateDevice(&adapter, config.feature_level, PR_COM_WRITE(mDevice)));

    // Capability checks

    // SM 6.0
    {
        D3D12_FEATURE_DATA_SHADER_MODEL feat_data = {D3D_SHADER_MODEL_6_0};
        auto const success = SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &feat_data, sizeof(feat_data)));
        mCapabilities.sm6 = success && feat_data.HighestShaderModel >= D3D_SHADER_MODEL_6_0;
    }

    // SM 6.0 wave intrinsics
    if (mCapabilities.sm6)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS1 feat_data = {};
        auto const success = SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &feat_data, sizeof(feat_data)));
        mCapabilities.sm6_wave_intrins = success && feat_data.WaveOps;
    }

    // DXR raytracing support and device query
    if (config.enable_dxr)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 feat_data = {};
        auto const success = SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feat_data, sizeof(feat_data)));
        if (success && feat_data.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            auto const query_success = SUCCEEDED(mDevice->QueryInterface(PR_COM_WRITE(mDeviceRaytracing)));
            mCapabilities.raytracing = query_success && mDeviceRaytracing.is_valid();
        }
    }
}
