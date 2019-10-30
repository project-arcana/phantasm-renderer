#pragma once
#ifdef PR_BACKEND_D3D12

#include <atomic>

#include "common/d3d12_sanitized.hh"

#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
class Fence
{
public:
    Fence(ID3D12Device& device, uint64_t initial_value);
    ~Fence();

    Fence(Fence const&) = delete;
    Fence& operator=(Fence const&) = delete;

    [[nodiscard]] ID3D12Fence* getRawFence() const { return mFence.get(); }
    [[nodiscard]] HANDLE getRawEvent() const { return mEvent; }

    [[nodiscard]] bool isAvailable() const { return fenceValueAvailableAt <= mFence->GetCompletedValue(); }
    uint64_t fenceValueAvailableAt = 0;

private:
    shared_com_ptr<ID3D12Fence> mFence;
    HANDLE mEvent;
};

}

#endif
