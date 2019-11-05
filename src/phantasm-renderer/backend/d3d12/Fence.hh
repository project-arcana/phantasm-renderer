#pragma once

#include <atomic>

#include "common/d3d12_fwd.hh"

#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
class Fence
{
public:
    Fence();
    ~Fence();

    void initialize(ID3D12Device& device, char const* debug_name = nullptr);

    Fence(Fence const&) = delete;
    Fence& operator=(Fence const&) = delete;
    Fence(Fence&&) noexcept = delete;
    Fence& operator=(Fence&&) noexcept = delete;

    void issueFence(ID3D12CommandQueue& queue);

    void waitOnCPU(uint64_t old_fence);
    void waitOnGPU(ID3D12CommandQueue& queue);

    [[nodiscard]] ID3D12Fence* getRawFence() const { return mFence.get(); }
    [[nodiscard]] HANDLE getRawEvent() const { return mEvent; }

private:
    shared_com_ptr<ID3D12Fence> mFence;
    HANDLE mEvent;
    uint64_t mCounter = 0;
};

}
