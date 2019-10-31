#include "Swapchain.hh"
#ifdef PR_BACKEND_D3D12

#include <phantasm-renderer/backend/detail/string_array_util.hh>

#include "common/d3d12_sanitized.hh"

#include "common/verify.hh"

void pr::backend::d3d12::Swapchain::initialize(IDXGIFactory4& factory, ID3D12Device& device, ID3D12CommandQueue& queue, HWND handle, unsigned num_backbuffers)
{
    CC_RUNTIME_ASSERT(mNumBackbuffers <= max_num_backbuffers);
    mNumBackbuffers = num_backbuffers;

    // Create fences
    {
        // Resize fences to the amount of backbuffers
        mFences.emplace(mNumBackbuffers);

        for (auto i = 0u; i < mFences.size(); ++i)
        {
            mFences[i].initialize(device, pr::backend::detail::formatted_stl_string("swapchain fence %i", i).c_str());
        }
    }

    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
    swapchain_desc.BufferCount = mNumBackbuffers;
    swapchain_desc.Width = 0;
    swapchain_desc.Height = 0;
    swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    shared_com_ptr<IDXGISwapChain1> temp_swapchain;
    PR_D3D12_VERIFY(factory.CreateSwapChainForHwnd(&queue, handle, &swapchain_desc, nullptr, nullptr, temp_swapchain.override()));
    PR_D3D12_VERIFY(temp_swapchain.get_interface(mSwapchain));
    PR_D3D12_VERIFY(factory.MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));
}

#endif
