#pragma once

#include <clean-core/capped_array.hh>

#include <typed-geometry/tg-lean.hh>

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

    void initialize(IDXGIFactory4& factory, shared_com_ptr<ID3D12Device> device, shared_com_ptr<ID3D12CommandQueue> queue, HWND handle, unsigned num_backbuffers);

    void onResize(int width, int height);
    void setFullscreen(bool fullscreen);

    void present();

    void waitForSwapchain();

    /// barrier the current backbuffer resource to the present state
    void barrierToPresent(ID3D12GraphicsCommandList* command_list);

    /// barrier the current backbuffer resource to the render target state
    void barrierToRenderTarget(ID3D12GraphicsCommandList* command_list);

public:
    [[nodiscard]] DXGI_FORMAT getBackbufferFormat() const;
    [[nodiscard]] tg::ivec2 const& getBackbufferSize() const { return mBackbufferSize; }

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE getCurrentBackbufferRTV() { return mBackbuffers[mSwapchain->GetCurrentBackBufferIndex()].rtv; }
    [[nodiscard]] ID3D12Resource* getCurrentBackbufferResource() const { return mBackbuffers[mSwapchain->GetCurrentBackBufferIndex()].resource; }

    [[nodiscard]] unsigned getNumBackbuffers() const { return unsigned(mBackbuffers.size()); }

    [[nodiscard]] IDXGISwapChain3& getSwapchain() const { return *mSwapchain.get(); }
    [[nodiscard]] shared_com_ptr<IDXGISwapChain3> const& getSwapchainShared() const { return mSwapchain; }

private:
    /// recreate RTVs, re-query resource pointers, reset state to present
    void updateBackbuffers();

    static auto constexpr max_num_backbuffers = 6u;

    shared_com_ptr<ID3D12Device> mParentDevice;            ///< The parent device
    shared_com_ptr<ID3D12CommandQueue> mParentDirectQueue; ///< The parent device's main direct queue
    shared_com_ptr<IDXGISwapChain3> mSwapchain;            ///< Swapchain COM ptr
    shared_com_ptr<ID3D12DescriptorHeap> mRTVHeap;         ///< A descriptor heap exclusively for backbuffer RTVs

    struct backbuffer
    {
        Fence fence;                     ///< present fence
        D3D12_CPU_DESCRIPTOR_HANDLE rtv; ///< CPU RTV
        ID3D12Resource* resource;        ///< resource ptr
        D3D12_RESOURCE_STATES state;     ///< current state
    };

    cc::capped_array<backbuffer, max_num_backbuffers> mBackbuffers; ///< All backbuffers

    tg::ivec2 mBackbufferSize;
};

}
