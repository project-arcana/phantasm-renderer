#include "Fence.hh"
#ifdef PR_BACKEND_D3D12

#include <corecrt_wstdio.h>

#include <clean-core/assert.hh>

#include "common/d3d12_sanitized.hh"
#include "common/verify.hh"

pr::backend::d3d12::Fence::Fence()
{
    mEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    CC_ASSERT(mEvent != INVALID_HANDLE_VALUE);
}

pr::backend::d3d12::Fence::~Fence()
{
    if (mEvent != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(mEvent);
    }
}

void pr::backend::d3d12::Fence::initialize(ID3D12Device& device, const char* debug_name)
{
    CC_ASSERT(!mFence.is_valid());

    PR_D3D12_VERIFY(device.CreateFence(0, D3D12_FENCE_FLAG_NONE, PR_COM_WRITE(mFence)));

    if (debug_name != nullptr)
    {
        wchar_t name_wide[1024];
        ::swprintf(name_wide, 1024, L"%S", debug_name);
        mFence->SetName(name_wide);
    }
}

void pr::backend::d3d12::Fence::issueFence(ID3D12CommandQueue& queue)
{
    ++mCounter;
    PR_D3D12_VERIFY(queue.Signal(mFence, mCounter));
}

void pr::backend::d3d12::Fence::waitOnCPU(uint64_t old_fence)
{
    if (mCounter > old_fence)
    {
        auto const valueToWaitFor = mCounter - old_fence;
        if (mFence->GetCompletedValue() <= valueToWaitFor)
        {
            PR_D3D12_VERIFY(mFence->SetEventOnCompletion(valueToWaitFor, mEvent));
            ::WaitForSingleObject(mEvent, INFINITE);
        }
    }
}

void pr::backend::d3d12::Fence::waitOnGPU(ID3D12CommandQueue& queue) { PR_D3D12_VERIFY(queue.Wait(mFence, mCounter)); }


#endif
