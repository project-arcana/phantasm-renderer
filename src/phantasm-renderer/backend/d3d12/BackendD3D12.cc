#include "BackendD3D12.hh"

#include "common/verify.hh"

void pr::backend::d3d12::BackendD3D12::initialize(const pr::backend::backend_config& config, HWND handle)
{
    mAdapter.initialize(config);
    mDevice.initialize(mAdapter.getAdapter(), config);
    mAllocator.initialize(mDevice.getDevice());
    mDirectQueue.initialize(mDevice.getDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    mSwapchain.initialize(mAdapter.getFactory(), mDevice.getDeviceShared(), mDirectQueue.getQueueShared(), handle, config.num_backbuffers);

    auto& device = mDevice.getDevice();
    mPoolResources.initialize(device, config.max_num_resources);
    mPoolCmdLists.initialize(*this, 10, 10); // TODO arbitrary
    mPoolPSOs.initialize(&device, config.max_num_pipeline_states);
    mPoolShaderViews.initialize(&device, &mPoolResources, int(config.max_num_shader_view_elements));

    mTranslator.initialize(&mDevice.getDevice(), &mPoolShaderViews, &mPoolResources, &mPoolPSOs);
}

void pr::backend::d3d12::BackendD3D12::destroy()
{
    flushGPU();
    mSwapchain.setFullscreen(false);

    mPoolPSOs.destroy();
    mPoolCmdLists.destroy();

    mAllocator.destroy();
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

pr::backend::handle::resource pr::backend::d3d12::BackendD3D12::acquireBackbuffer()
{
    auto const backbuffer_i = mSwapchain.waitForBackbuffer();
    auto const& backbuffer = mSwapchain.getBackbuffer(backbuffer_i);
    return mPoolResources.injectBackbufferResource(
        backbuffer.resource, backbuffer.state == D3D12_RESOURCE_STATE_RENDER_TARGET ? resource_state::render_target : resource_state::present);
}

pr::backend::handle::command_list pr::backend::d3d12::BackendD3D12::recordCommandList(std::byte* buffer, size_t size)
{
    ID3D12GraphicsCommandList* raw_list;
    auto const res = mPoolCmdLists.create(raw_list);
    mTranslator.translateCommandList(raw_list, mPoolCmdLists.getStateCache(res), buffer, size);
    return res;
}

void pr::backend::d3d12::BackendD3D12::submit(pr::backend::handle::command_list cl)
{
    // TODO: Batching
    // TODO: State cache
    mDirectQueue.submit(mPoolCmdLists.getRawList(cl));
    mPoolCmdLists.freeOnSubmit(cl, mDirectQueue.getQueue());
}
