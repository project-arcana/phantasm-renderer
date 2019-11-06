#pragma once
#ifdef PR_BACKEND_D3D12

#include <phantasm-renderer/backend/BackendInterface.hh>

#include "Adapter.hh"
#include "Device.hh"
#include "Queue.hh"
#include "Swapchain.hh"

namespace pr::backend::d3d12
{
class BackendD3D12 final : public BackendInterface
{
public:
    void initialize(d3d12_config const& config, HWND handle);

    void flushGPU();

    Adapter mAdapter;
    Device mDevice;
    Queue mDirectQueue;
    Swapchain mSwapchain;
private:
};
}

#endif
