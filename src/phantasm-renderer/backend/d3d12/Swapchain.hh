#pragma once
#ifdef PR_BACKEND_D3D12

#include "common/shared_com_ptr.hh"
#include "common/d3d12_fwd.hh"

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

    void initialize(IDXGIFactory4& factory, ID3D12CommandQueue& queue, HWND handle);

    [[nodiscard]] IDXGISwapChain3& getSwapchain() const { return *mSwapchain.get(); }
    [[nodiscard]] shared_com_ptr<IDXGISwapChain3> const& getSwapchainShared() const { return mSwapchain; }

private:
    shared_com_ptr<IDXGISwapChain3> mSwapchain;
};

}

#endif
