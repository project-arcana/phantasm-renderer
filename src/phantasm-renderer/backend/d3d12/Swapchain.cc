#include "Swapchain.hh"
#ifdef PR_BACKEND_D3D12

#include "common/verify.hh"

void pr::backend::d3d12::Swapchain::initialize(IDXGIFactory4 &factory, HWND handle)
{
//    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
//        swapChainDesc.BufferCount = FrameCount;
//        swapChainDesc.Width = m_width;
//        swapChainDesc.Height = m_height;
//        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//        swapChainDesc.SampleDesc.Count = 1;

//        ComPtr<IDXGISwapChain1> swapChain;
//        ThrowIfFailed(factory->CreateSwapChainForHwnd(
//            m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
//            Win32Application::GetHwnd(),
//            &swapChainDesc,
//            nullptr,
//            nullptr,
//            &swapChain
//            ));
}

#endif
