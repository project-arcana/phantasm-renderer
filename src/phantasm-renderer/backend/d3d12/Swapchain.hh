#pragma once

#include <clean-core/capped_array.hh>

#include <typed-geometry/tg-lean.hh>

#include <phantasm-renderer/backend/types.hh>

#include "common/d3d12_sanitized.hh"
#include "common/shared_com_ptr.hh"

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

    void initialize(IDXGIFactory4& factory, shared_com_ptr<ID3D12Device> device, shared_com_ptr<ID3D12CommandQueue> queue, HWND handle, unsigned num_backbuffers, present_mode present_mode);
    ~Swapchain();

    void onResize(int width, int height);
    void setFullscreen(bool fullscreen);

    /// call swapchain->Present(0, 0) and issue the fence
    void present();

    /// wait for the fence of the current backbuffer
    unsigned waitForBackbuffer();

public:
    struct backbuffer
    {
        Fence fence;                     ///< present fence
        D3D12_CPU_DESCRIPTOR_HANDLE rtv; ///< CPU RTV
        ID3D12Resource* resource;        ///< resource ptr
        D3D12_RESOURCE_STATES state;     ///< current state
    };

    [[nodiscard]] DXGI_FORMAT getBackbufferFormat() const;
    [[nodiscard]] tg::ivec2 const& getBackbufferSize() const { return mBackbufferSize; }

    [[nodiscard]] backbuffer const& getBackbuffer(unsigned i) const { return mBackbuffers[i]; }

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackbufferRTV() const { return mBackbuffers[mSwapchain->GetCurrentBackBufferIndex()].rtv; }
    [[nodiscard]] ID3D12Resource* getCurrentBackbufferResource() const { return mBackbuffers[mSwapchain->GetCurrentBackBufferIndex()].resource; }

    [[nodiscard]] unsigned getNumBackbuffers() const { return unsigned(mBackbuffers.size()); }

    [[nodiscard]] IDXGISwapChain3& getSwapchain() const { return *mSwapchain.get(); }
    [[nodiscard]] shared_com_ptr<IDXGISwapChain3> const& getSwapchainShared() const { return mSwapchain; }

private:
    /// recreate RTVs, re-query resource pointers, reset state to present
    void updateBackbuffers();

    void releaseBackbuffers();

private:
    static auto constexpr max_num_backbuffers = 6u;

    shared_com_ptr<ID3D12Device> mParentDevice;            ///< The parent device
    shared_com_ptr<ID3D12CommandQueue> mParentDirectQueue; ///< The parent device's main direct queue
    shared_com_ptr<IDXGISwapChain3> mSwapchain;            ///< Swapchain COM ptr
    shared_com_ptr<ID3D12DescriptorHeap> mRTVHeap;         ///< A descriptor heap exclusively for backbuffer RTVs


    cc::capped_array<backbuffer, max_num_backbuffers> mBackbuffers; ///< All backbuffers

    tg::ivec2 mBackbufferSize;
    present_mode mPresentMode;
};

}
