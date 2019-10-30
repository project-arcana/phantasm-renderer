#include "Swapchain.hh"
#ifdef PR_BACKEND_D3D12

#include "common/verify.hh"

void pr::backend::d3d12::Swapchain::initialize(IDXGIFactory4& factory, ID3D12CommandQueue& queue, HWND handle)
{
    // TODO: configurable
    auto constexpr num_backbuffers = 2;
    auto constexpr backbuffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;

    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
    swapchain_desc.BufferCount = num_backbuffers;
    swapchain_desc.Width = 0;
    swapchain_desc.Height = 0;
    swapchain_desc.Format = backbuffer_format;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.SampleDesc.Count = 1;

    shared_com_ptr<IDXGISwapChain1> temp_swapchain;
    PR_D3D12_VERIFY(factory.CreateSwapChainForHwnd(&queue, handle, &swapchain_desc, nullptr, nullptr, temp_swapchain.override()));
    PR_D3D12_VERIFY(temp_swapchain.get_interface(mSwapchain));
}

#endif
