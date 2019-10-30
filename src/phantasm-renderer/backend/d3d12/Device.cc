#include "Device.hh"
#ifdef PR_BACKEND_D3D12

#include "common/verify.hh"

void pr::backend::d3d12::Device::initialize(IDXGIAdapter& adapter, const pr::backend::d3d12::d3d12_config& config)
{
    PR_D3D12_VERIFY(::D3D12CreateDevice(&adapter, config.feature_level, PR_COM_WRITE(mDevice)));

    // Check for Shader Model 6.0 wave intrinsics support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS1 feature_data;
        bool const feature_check_success = SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_data, sizeof(feature_data)));

        if (feature_check_success && feature_data.WaveOps)
        {
            mHasSM6WaveIntrinsics = true;
        }
    }

    // Check for DXR raytracing support
    if (config.enable_dxr)
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_data;
        bool const feature_check_success = SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_data, sizeof(feature_data)));

        if (feature_check_success && feature_data.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
        {
            mDevice->QueryInterface(PR_COM_WRITE(mDeviceRaytracing));
        }
    }
}

#endif
