#pragma once

#include <atomic>

#include "common/d3d12_sanitized.hh"

#include "common/shared_com_ptr.hh"

namespace pr::backend::d3d12
{
class SimpleFence
{
public:
    void initialize(ID3D12Device& device, char const* debug_name = nullptr);
    ~SimpleFence();

    SimpleFence(SimpleFence const&) = delete;
    SimpleFence& operator=(SimpleFence const&) = delete;
    SimpleFence(SimpleFence&&) noexcept = delete;
    SimpleFence& operator=(SimpleFence&&) noexcept = delete;

    void signalCPU(uint64_t new_val);
    void signalGPU(uint64_t new_val, ID3D12CommandQueue& queue);

    void waitCPU(uint64_t val);
    void waitGPU(uint64_t val, ID3D12CommandQueue& queue);

    [[nodiscard]] uint64_t getCurrentValue() const { return mFence->GetCompletedValue(); }

    [[nodiscard]] ID3D12Fence* getRawFence() const { return mFence.get(); }
    [[nodiscard]] HANDLE getRawEvent() const { return mEvent; }

private:
    shared_com_ptr<ID3D12Fence> mFence;
    HANDLE mEvent;
};

class Fence
{
public:
    void initialize(ID3D12Device& device, char const* debug_name = nullptr) { mFence.initialize(device, debug_name); }

    void issueFence(ID3D12CommandQueue& queue)
    {
        ++mCounter;
        mFence.signalGPU(mCounter, queue);
    }

    void waitOnCPU(uint64_t old_fence)
    {
        if (mCounter > old_fence)
        {
            mFence.waitCPU(mCounter - old_fence);
        }
    }

    void waitOnGPU(ID3D12CommandQueue& queue)
    {
        mFence.waitGPU(mCounter, queue);
    }

private:
    SimpleFence mFence;
    uint64_t mCounter = 0;
};

}
