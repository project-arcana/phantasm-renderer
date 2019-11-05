#pragma once

#include "common/d3d12_fwd.hh"

#include "common/shared_com_ptr.hh"
#include "d3d12_config.hh"

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

    void initialize(IDXGIAdapter& adapter, d3d12_config const& config);

    [[nodiscard]] bool hasSM6WaveIntrinsics() const { return mHasSM6WaveIntrinsics; }
    [[nodiscard]] bool hasRaytracing() const { return mDeviceRaytracing.is_valid(); }

    [[nodiscard]] ID3D12Device& getDevice() const { return *mDevice.get(); }
    [[nodiscard]] shared_com_ptr<ID3D12Device> const& getDeviceShared() const { return mDevice; }

private:
    shared_com_ptr<ID3D12Device> mDevice;
    shared_com_ptr<ID3D12Device5> mDeviceRaytracing;
    bool mHasSM6WaveIntrinsics = false; // TODO: Bitset for capabilities
};

}
