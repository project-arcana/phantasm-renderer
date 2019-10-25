#include "Adapter.hh"
#ifdef PR_BACKEND_D3D12

#include "common/verify.hh"

void pr::backend::d3d12::Adapter::initialize(d3d12_config const& config)
{
    // Factory init
    {
        PR_D3D12_VERIFY(::CreateDXGIFactory(PR_COM_WRITE(mDXGIFactory)));
        PR_D3D12_VERIFY(mDXGIFactory->QueryInterface(PR_COM_WRITE(mDXGIFactoryV2)));
    }

    // Adapter init
    {
        shared_com_ptr<IDXGIAdapter> temp_adapter;
        mDXGIFactory->EnumAdapters(0, temp_adapter.override());
        PR_D3D12_VERIFY(temp_adapter->QueryInterface(PR_COM_WRITE(mDXGIAdapter)));
    }

    // Debug layer init
    if (config.enable_validation)
    {
        shared_com_ptr<ID3D12Debug> debug_controller;
        bool const debug_init_success = SUCCEEDED(::D3D12GetDebugInterface(PR_COM_WRITE(debug_controller)));

        if (debug_init_success)
        {
            debug_controller->EnableDebugLayer();

            if (config.enable_gpu_validation)
            {
                shared_com_ptr<ID3D12Debug3> debug_controller_v3;
                PR_D3D12_VERIFY(debug_controller->QueryInterface(PR_COM_WRITE(debug_controller_v3)));
                debug_controller_v3->SetEnableGPUBasedValidation(true);
            }
        }
        else
        {
            // TODO: Log that the D3D12 SDK is missing
            // refer to https://docs.microsoft.com/en-us/windows/uwp/gaming/use-the-directx-runtime-and-visual-studio-graphics-diagnostic-features
        }
    }

    // Root device init
    {
        // TODO: Feature level
        PR_D3D12_VERIFY(::D3D12CreateDevice(mDXGIAdapter, config.feature_level, PR_COM_WRITE(mRootDevice)));

        // Check for Shader Model 6.0 wave intrinsics support
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS1 feature_data;
            bool const feature_check_success = SUCCEEDED(mRootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_data, sizeof(feature_data)));

            if (feature_check_success && feature_data.WaveOps)
            {
                mCapabilities.has_sm6_wave_intrinsics = true;
            }
        }

        // Check for DXR raytracing support
        if (config.enable_dxr)
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 feature_data;
            bool const feature_check_success = SUCCEEDED(mRootDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &feature_data, sizeof(feature_data)));

            if (feature_check_success && feature_data.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0)
            {
                mRootDevice->QueryInterface(PR_COM_WRITE(mRootDeviceV5));
                if (mRootDeviceV5.is_valid())
                {
                    mCapabilities.has_raytracing = true;
                }
            }
        }
    }
}


#endif
