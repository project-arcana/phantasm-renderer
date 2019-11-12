#include "Swapchain.hh"

#include <iostream>

#include <phantasm-renderer/backend/detail/string_array_util.hh>

#include "common/verify.hh"

namespace
{
// NOTE: The _SRGB variant crashes at factory.CreateSwapChainForHwnd
static constexpr auto s_backbuffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
static constexpr auto s_swapchain_flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
}

void pr::backend::d3d12::Swapchain::initialize(IDXGIFactory4& factory, shared_com_ptr<ID3D12Device> device, shared_com_ptr<ID3D12CommandQueue> queue, HWND handle, unsigned num_backbuffers)
{
    CC_RUNTIME_ASSERT(mNumBackbuffers <= max_num_backbuffers);
    mNumBackbuffers = num_backbuffers;
    mParentDevice = cc::move(device);
    mParentDirectQueue = cc::move(queue);

    // Create fences
    {
        // Resize fences to the amount of backbuffers
        mFences.emplace(mNumBackbuffers);

        for (auto i = 0u; i < mFences.size(); ++i)
        {
            mFences[i].initialize(*mParentDevice, pr::backend::detail::formatted_stl_string("swapchain fence %i", i).c_str());
        }
    }

    // Create swapchain
    {
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        swapchain_desc.BufferCount = mNumBackbuffers;
        swapchain_desc.Width = 0;
        swapchain_desc.Height = 0;
        swapchain_desc.Format = s_backbuffer_format;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.Flags = s_swapchain_flags;

        shared_com_ptr<IDXGISwapChain1> temp_swapchain;
        PR_D3D12_VERIFY(factory.CreateSwapChainForHwnd(mParentDirectQueue, handle, &swapchain_desc, nullptr, nullptr, temp_swapchain.override()));
        PR_D3D12_VERIFY(temp_swapchain.get_interface(mSwapchain));
    }

    // Disable Alt + Enter behavior
    PR_D3D12_VERIFY(factory.MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));

    // Create backbuffer RTV heap, then create RTVs
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
        rtv_heap_desc.NumDescriptors = mNumBackbuffers;
        rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtv_heap_desc.NodeMask = 0;

        PR_D3D12_VERIFY(mParentDevice->CreateDescriptorHeap(&rtv_heap_desc, PR_COM_WRITE(mRTVHeap)));

        createBackbufferRTVs();
    }
}

void pr::backend::d3d12::Swapchain::onResize(int width, int height)
{
    mBackbufferSize = tg::ivec2(width, height);
    PR_D3D12_VERIFY(mSwapchain->ResizeBuffers(mNumBackbuffers, UINT(width), UINT(height), s_backbuffer_format, s_swapchain_flags));
    createBackbufferRTVs();
}

void pr::backend::d3d12::Swapchain::setFullscreen(bool fullscreen) { PR_D3D12_VERIFY(mSwapchain->SetFullscreenState(fullscreen, nullptr)); }

void pr::backend::d3d12::Swapchain::present()
{
    PR_D3D12_VERIFY(mSwapchain->Present(0, 0));

    auto const backbuffer_i = mSwapchain->GetCurrentBackBufferIndex();
    mFences[backbuffer_i].issueFence(*mParentDirectQueue);
}

void pr::backend::d3d12::Swapchain::waitForSwapchain()
{
    auto const backbuffer_i = mSwapchain->GetCurrentBackBufferIndex();
    mFences[backbuffer_i].waitOnCPU(0);
}

DXGI_FORMAT pr::backend::d3d12::Swapchain::getBackbufferFormat() const { return s_backbuffer_format; }

ID3D12Resource* pr::backend::d3d12::Swapchain::getCurrentBackbufferResource() const
{
    auto const backbuffer_i = mSwapchain->GetCurrentBackBufferIndex();

    ID3D12Resource* res;
    PR_D3D12_VERIFY(mSwapchain->GetBuffer(backbuffer_i, IID_PPV_ARGS(&res)));
    // NOTE: This is correct, based on AMD utils implementation
    res->Release();
    return res;
}

D3D12_CPU_DESCRIPTOR_HANDLE& pr::backend::d3d12::Swapchain::getCurrentBackbufferRTV() { return mRTVs[mSwapchain->GetCurrentBackBufferIndex()]; }

void pr::backend::d3d12::Swapchain::createBackbufferRTVs()
{
    auto const rtv_size = mParentDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mRTVs.emplace(mNumBackbuffers);

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.Format = s_backbuffer_format;
    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D.MipSlice = 0;
    rtv_desc.Texture2D.PlaneSlice = 0;

    for (auto i = 0u; i < mNumBackbuffers; ++i)
    {
        auto& rtv = mRTVs[i];

        rtv = mRTVHeap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += rtv_size * i;

        shared_com_ptr<ID3D12Resource> back_buffer;
        PR_D3D12_VERIFY(mSwapchain->GetBuffer(i, PR_COM_WRITE(back_buffer)));

        mParentDevice->CreateRenderTargetView(back_buffer, &rtv_desc, rtv);
    }
}
