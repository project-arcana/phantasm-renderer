#pragma once
#ifdef PR_BACKEND_D3D12

#include <d3d12.h>
#include <dxgi1_6.h>

#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
struct adapter_capabilities
{
    bool has_sm6_wave_intrinsics = false;
    bool has_raytracing = false;
};

/// Represents a IDXGIAdapter, the uppermost object in the D3D12 hierarchy
class Adapter
{
public:
    Adapter() = default;
    void initialize();

    adapter_capabilities const& getCapabilities() const { return mCapabilities; }

private:
    shared_com_ptr<IDXGIAdapter> mDXGIAdapter;

    shared_com_ptr<IDXGIFactory> mDXGIFactory;
    shared_com_ptr<IDXGIFactory2> mDXGIFactoryV2;

    shared_com_ptr<ID3D12Device> mRootDevice;
    shared_com_ptr<ID3D12Device5> mRootDeviceV5;

    adapter_capabilities mCapabilities;

private:
    Adapter(Adapter const&) = delete;
    Adapter& operator=(Adapter const&) = delete;
};

}

#endif
