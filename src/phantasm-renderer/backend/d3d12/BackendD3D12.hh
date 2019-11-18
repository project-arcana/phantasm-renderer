#pragma once

#include <phantasm-renderer/backend/Backend.hh>
#include <phantasm-renderer/backend/types.hh>

#include "Adapter.hh"
#include "Device.hh"
#include "Queue.hh"
#include "Swapchain.hh"
#include "resources/resource.hh"

namespace pr::backend::d3d12
{
class BackendD3D12 final : public Backend
{
public:
    void initialize(backend_config const& config, HWND handle);

    /// flush all pending work on the GPU
    void flushGPU();

    void resize(int w, int h);

public:
    Adapter mAdapter;
    Device mDevice;
    Queue mDirectQueue;
    Swapchain mSwapchain;
    ResourceAllocator mAllocator;

private:
};
}
