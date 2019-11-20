#include "Fence.hh"

#include <corecrt_wstdio.h>

#include <clean-core/assert.hh>

#include "common/d3d12_sanitized.hh"
#include "common/verify.hh"


void pr::backend::d3d12::SimpleFence::initialize(ID3D12Device& device, const char* debug_name)
{
    CC_ASSERT(!mFence.is_valid());

    mEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
    CC_ASSERT(mEvent != INVALID_HANDLE_VALUE);

    PR_D3D12_VERIFY(device.CreateFence(0, D3D12_FENCE_FLAG_NONE, PR_COM_WRITE(mFence)));

    if (debug_name != nullptr)
    {
        wchar_t name_wide[1024];
        ::swprintf(name_wide, 1024, L"%S", debug_name);
        mFence->SetName(name_wide);
    }
}

pr::backend::d3d12::SimpleFence::~SimpleFence()
{
    if (mEvent != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(mEvent);
    }
}


void pr::backend::d3d12::SimpleFence::waitCPU(uint64_t val)
{
    if (mFence->GetCompletedValue() <= val)
    {
        PR_D3D12_VERIFY(mFence->SetEventOnCompletion(val, mEvent));
        ::WaitForSingleObject(mEvent, INFINITE);
    }
}

void pr::backend::d3d12::SimpleFence::waitGPU(uint64_t val, ID3D12CommandQueue& queue)
{
    //
    PR_D3D12_VERIFY(queue.Wait(mFence, val));
}
