#include "BackendD3D12.hh"

#include <clean-core/vector.hh>

#include <phantasm-renderer/backend/device_tentative/window.hh>

#include "cmd_list_translation.hh"
#include "common/native_enum.hh"
#include "common/verify.hh"

namespace pr::backend::d3d12
{
struct BackendD3D12::per_thread_component
{
    command_list_translator translator;
    CommandAllocatorBundle cmd_list_allocator;
};

}

void pr::backend::d3d12::BackendD3D12::initialize(const pr::backend::backend_config& config, device::Window& window)
{
    // Core components
    {
        mAdapter.initialize(config);
        mDevice.initialize(mAdapter.getAdapter(), config);
        mDirectQueue.initialize(mDevice.getDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
        mSwapchain.initialize(mAdapter.getFactory(), mDevice.getDeviceShared(), mDirectQueue.getQueueShared(), window.getHandle(), config.num_backbuffers);
    }

    auto& device = mDevice.getDevice();

    // Global pools
    {
        mPoolResources.initialize(device, config.max_num_resources);
        mPoolPSOs.initialize(&device, config.max_num_pipeline_states);
        mPoolShaderViews.initialize(&device, &mPoolResources, int(config.max_num_shader_view_elements));
    }

    // Per-thread components and command list pool
    {
        mThreadAssociation.initialize();
        mThreadComponents = mThreadComponents.defaulted(config.num_threads);

        cc::vector<CommandAllocatorBundle*> thread_allocator_ptrs;
        thread_allocator_ptrs.reserve(config.num_threads);

        for (auto& thread_comp : mThreadComponents)
        {
            thread_comp.translator.initialize(&device, &mPoolShaderViews, &mPoolResources, &mPoolPSOs);
            thread_allocator_ptrs.push_back(&thread_comp.cmd_list_allocator);
        }

        // TODO arbitrary
        mPoolCmdLists.initialize(*this, config.num_cmdlist_allocators_per_thread, config.num_cmdlists_per_allocator, thread_allocator_ptrs);
    }
}

pr::backend::d3d12::BackendD3D12::~BackendD3D12()
{
    flushGPU();
    mSwapchain.setFullscreen(false);

    mPoolPSOs.destroy();
    mPoolCmdLists.destroy();
    mPoolResources.destroy();

    for (auto& thread_comp : mThreadComponents)
    {
        thread_comp.cmd_list_allocator.destroy();
    }
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
    auto& thread_comp = mThreadComponents[mThreadAssociation.get_current_index()];

    ID3D12GraphicsCommandList* raw_list;
    auto const res = mPoolCmdLists.create(raw_list, thread_comp.cmd_list_allocator);
    thread_comp.translator.translateCommandList(raw_list, mPoolCmdLists.getStateCache(res), buffer, size);
    return res;
}

void pr::backend::d3d12::BackendD3D12::submit(cc::span<const pr::backend::handle::command_list> cls)
{
    CC_RUNTIME_ASSERT(cls.size() <= 12 && "submit too large");

    cc::capped_vector<ID3D12CommandList*, 24> submit_batch;
    cc::capped_vector<handle::command_list, 12> barrier_lists;

    auto& thread_comp = mThreadComponents[mThreadAssociation.get_current_index()];

    for (auto const& cl : cls)
    {
        if (cl == handle::null_command_list)
            continue;

        auto const* const state_cache = mPoolCmdLists.getStateCache(cl);
        cc::capped_vector<D3D12_RESOURCE_BARRIER, 32> barriers;

        for (auto const& entry : state_cache->cache)
        {
            auto const master_before = mPoolResources.getResourceState(entry.ptr);

            if (master_before != entry.required_initial)
            {
                // transition to the state required as the initial one
                auto& barrier = barriers.emplace_back();
                util::populate_barrier_desc(barrier, mPoolResources.getRawResource(entry.ptr), util::to_native(master_before),
                                            util::to_native(entry.required_initial));
            }

            // set the master state to the one in which this resource is left
            mPoolResources.setResourceState(entry.ptr, entry.current);
        }

        if (!barriers.empty())
        {
            ID3D12GraphicsCommandList* t_cmd_list;
            barrier_lists.push_back(mPoolCmdLists.create(t_cmd_list, thread_comp.cmd_list_allocator));
            t_cmd_list->ResourceBarrier(UINT(barriers.size()), barriers.size() > 0 ? barriers.data() : nullptr);
            t_cmd_list->Close();
            submit_batch.push_back(t_cmd_list);
        }

        submit_batch.push_back(mPoolCmdLists.getRawList(cl));
    }


    auto& queue = mDirectQueue.getQueue();
    queue.ExecuteCommandLists(UINT(submit_batch.size()), submit_batch.data());
    mPoolCmdLists.freeOnSubmit(barrier_lists, queue);
    mPoolCmdLists.freeOnSubmit(cls, queue);
}
