#include "BackendD3D12.hh"

#include "common/verify.hh"

void pr::backend::d3d12::BackendD3D12::initialize(const pr::backend::d3d12::d3d12_config& config, HWND handle)
{
    mAdapter.initialize(config);
    mDevice.initialize(mAdapter.getAdapter(), config);
    mAllocator.initialize(mDevice.getDevice());
    mDirectQueue.initialize(mDevice.getDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    mSwapchain.initialize(mAdapter.getFactory(), mDevice.getDeviceShared(), mDirectQueue.getQueueShared(), handle, config.num_backbuffers);
}

void pr::backend::d3d12::BackendD3D12::flushGPU()
{
    shared_com_ptr<ID3D12Fence> fence;
    PR_D3D12_VERIFY(mDevice.getDevice().CreateFence(0, D3D12_FENCE_FLAG_NONE, PR_COM_WRITE(fence)));
    PR_D3D12_VERIFY(mDirectQueue.getQueue().Signal(fence, 1));

    auto const handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    fence->SetEventOnCompletion(1, handle);
    ::WaitForSingleObject(handle, INFINITE);
    ::CloseHandle(handle);
}
