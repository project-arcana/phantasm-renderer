#pragma once

#include <phantasm-renderer/backend/Backend.hh>
#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/backend/detail/thread_association.hh>

#include "Device.hh"
#include "Swapchain.hh"
#include "loader/volk.hh"
#include "memory/Allocator.hh"
#include "pools/cmd_list_pool.hh"

namespace pr::backend::device
{
class Window;
};

namespace pr::backend::vk
{
class BackendVulkan// final : public Backend
{
public:
    void initialize(backend_config const& config, device::Window& window);
    ~BackendVulkan() /*override*/;

    //
    // Command list interface
    //

    [[nodiscard]] handle::command_list recordCommandList(std::byte* buffer, size_t size) /*override*/;

    void discard(handle::command_list cl) /*override*/ { mPoolCmdLists.freeOnDiscard(cl); }

    void submit(cc::span<handle::command_list const> cls) /*override*/;

public:
    // backend-internal

    /// flush all pending work on the GPU
    void flushGPU();

private:

    void createDebugMessenger();

public:
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    Device mDevice;
    Swapchain mSwapchain;
    Allocator mAllocator;

public:
    // Pools
//    ResourcePool mPoolResources;
    CommandListPool mPoolCmdLists;
//    PipelineStateObjectPool mPoolPSOs;
//    ShaderViewPool mPoolShaderViews;

    // Logic
    struct per_thread_component;
    cc::array<per_thread_component> mThreadComponents;
    backend::detail::thread_association mThreadAssociation;
};
}

