#pragma once
#ifdef PR_BACKEND_D3D12

#include <clean-core/capped_array.hh>

#include "common/shared_com_ptr.hh"
#include "common/d3d12_fwd.hh"

#include "Fence.hh"

namespace pr::backend::d3d12
{

class Swapchain
{
    // reference type
public:
    Swapchain() = default;
    Swapchain(Swapchain const&) = delete;
    Swapchain(Swapchain&&) noexcept = delete;
    Swapchain& operator=(Swapchain const&) = delete;
    Swapchain& operator=(Swapchain&&) noexcept = delete;

    void initialize(IDXGIFactory4& factory, ID3D12Device& device, ID3D12CommandQueue& queue, HWND handle, unsigned num_backbuffers);

    [[nodiscard]] IDXGISwapChain3& getSwapchain() const { return *mSwapchain.get(); }
    [[nodiscard]] shared_com_ptr<IDXGISwapChain3> const& getSwapchainShared() const { return mSwapchain; }

private:
    static auto constexpr max_num_backbuffers = 6u;

    shared_com_ptr<IDXGISwapChain3> mSwapchain;
    cc::capped_array<Fence, max_num_backbuffers> mFences;
    unsigned mNumBackbuffers = 0;
};

}

#endif
