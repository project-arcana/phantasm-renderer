#pragma once

#include <phantasm-renderer/backend/types.hh>

#include "common/d3d12_fwd.hh"
#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
class Device
{
    // reference type
public:
    Device() = default;
    Device(Device const&) = delete;
    Device(Device&&) noexcept = delete;
    Device& operator=(Device const&) = delete;
    Device& operator=(Device&&) noexcept = delete;

    void initialize(IDXGIAdapter& adapter, backend_config const& config);

    bool hasSM6() const { return mCapabilities.sm6; }
    bool hasSM6WaveIntrinsics() const { return mCapabilities.sm6_wave_intrins; }
    bool hasRaytracing() const { return mCapabilities.raytracing; }

    ID3D12Device& getDevice() const { return *mDevice.get(); }
    shared_com_ptr<ID3D12Device> const& getDeviceShared() const { return mDevice; }

    ID3D12Device5* getDeviceRaytracing() const { return mDeviceRaytracing.get(); }

private:
    shared_com_ptr<ID3D12DeviceRemovedExtendedDataSettings> mDREDSettings;
    shared_com_ptr<ID3D12Device> mDevice;
    shared_com_ptr<ID3D12Device5> mDeviceRaytracing;

    struct capabilities
    {
        bool sm6 = false;
        bool sm6_wave_intrins = false;
        bool raytracing = false;
    } mCapabilities;
};

}
