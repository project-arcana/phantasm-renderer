#include "Fence.hh"
#ifdef PR_BACKEND_D3D12

#include <clean-core/assert.hh>

#include "common/verify.hh"

pr::backend::d3d12::Fence::Fence(ID3D12Device& device, uint64_t initial_value)
{
    mEvent = CreateEvent(nullptr, false, false, nullptr);
    CC_ASSERT(mEvent != INVALID_HANDLE_VALUE);
    PR_D3D12_VERIFY(device.CreateFence(initial_value, D3D12_FENCE_FLAG_NONE, PR_COM_WRITE(mFence)));
}

pr::backend::d3d12::Fence::~Fence()
{
    if (mEvent != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(mEvent);
    }
}


#endif
