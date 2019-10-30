#pragma once
#ifdef PR_BACKEND_D3D12

#include <clean-core/native/win32_fwd.hh>

// Forward declarations of common D3D12 entities
// extend as needed

typedef unsigned __int64 UINT64;
typedef UINT64 D3D12_GPU_VIRTUAL_ADDRESS;

struct ID3D12Device;
struct ID3D12Device5;
struct ID3D12Fence;
struct ID3D12Resource;
struct ID3D12CommandQueue;

struct IDXGIAdapter;
struct IDXGIFactory4;
struct IDXGISwapChain3;

#endif
