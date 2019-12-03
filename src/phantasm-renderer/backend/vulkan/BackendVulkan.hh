#pragma once

#include <phantasm-renderer/backend/Backend.hh>
#include <phantasm-renderer/backend/detail/thread_association.hh>
#include <phantasm-renderer/backend/types.hh>

#include "Device.hh"
#include "Swapchain.hh"
#include "loader/volk.hh"
#include "memory/ResourceAllocator.hh"

#include "pools/cmd_list_pool.hh"
#include "pools/pipeline_pool.hh"
#include "pools/resource_pool.hh"
#include "pools/shader_view_pool.hh"

namespace pr::backend::device
{
class Window;
};

namespace pr::backend::vk
{
class BackendVulkan final : public Backend
{
public:
    void initialize(backend_config const& config, device::Window& window);
    ~BackendVulkan() override;

public:
    // Virtual interface

    //
    // Swapchain interface
    //

    [[nodiscard]] handle::resource acquireBackbuffer() override;

    void present() override
    {
        mSwapchain.performPresentSubmit();
        mSwapchain.present();
    }

    void resize(int w, int h) override { mSwapchain.onResize(w, h); }

    [[nodiscard]] tg::ivec2 getBackbufferSize() const override { return mSwapchain.getBackbufferSize(); }

    [[nodiscard]] format getBackbufferFormat() const override;


    //
    // Resource interface
    //

    /// create a 2D texture2
    [[nodiscard]] handle::resource createTexture2D(backend::format format, int w, int h, int mips) override
    {
        return mPoolResources.createTexture2D(format, w, h, mips);
    }

    /// create a render- or depth-stencil target
    [[nodiscard]] handle::resource createRenderTarget(backend::format format, int w, int h) override
    {
        return mPoolResources.createRenderTarget(format, w, h);
    }

    /// create a buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createBuffer(unsigned size_bytes, resource_state initial_state, unsigned stride_bytes = 0) override
    {
        return mPoolResources.createBuffer(size_bytes, initial_state, stride_bytes);
    }

    /// create a mapped UPLOAD_HEAP buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createMappedBuffer(unsigned size_bytes, unsigned stride_bytes = 0) override
    {
        return mPoolResources.createMappedBuffer(size_bytes, stride_bytes);
    }

    void free(handle::resource res) override { mPoolResources.free(res); }

    [[nodiscard]] std::byte* getMappedMemory(handle::resource res) override { return mPoolResources.getMappedMemory(res); }

    //
    // Shader view interface
    //

    [[nodiscard]] handle::shader_view createShaderView(cc::span<shader_view_element const> srvs, cc::span<shader_view_element const> uavs = {}) override
    {
        return mPoolShaderViews.create(srvs, uavs);
    }

    void free(handle::shader_view sv) override { mPoolShaderViews.free(sv); }

    //
    // Pipeline state interface
    //

    [[nodiscard]] handle::pipeline_state createPipelineState(arg::vertex_format vertex_format,
                                                             arg::framebuffer_format framebuffer_format,
                                                             arg::shader_argument_shapes shader_arg_shapes,
                                                             arg::shader_sampler_configs shader_samplers,
                                                             arg::shader_stages shader_stages,
                                                             pr::primitive_pipeline_config const& primitive_config) override
    {
        return mPoolPipelines.createPipelineState(vertex_format, framebuffer_format, shader_arg_shapes, shader_samplers, shader_stages, primitive_config);
    }

    void free(handle::pipeline_state ps) override { mPoolPipelines.free(ps); }

    //
    // Command list interface
    //

    [[nodiscard]] handle::command_list recordCommandList(std::byte* buffer, size_t size) override;

    void discard(handle::command_list cl) override { mPoolCmdLists.freeOnDiscard(cl); }

    void submit(cc::span<handle::command_list const> cls) override;


public:
    // backend-internal

    /// flush all pending work on the GPU
    void flushGPU();

private:
    void createDebugMessenger();

public:
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger = nullptr;
    VkSurfaceKHR mSurface = nullptr;
    Device mDevice;
    Swapchain mSwapchain;
    ResourceAllocator mAllocator;

public:
    // Pools
    ResourcePool mPoolResources;
    CommandListPool mPoolCmdLists;
    PipelinePool mPoolPipelines;
    ShaderViewPool mPoolShaderViews;

    // Logic
    struct per_thread_component;
    cc::array<per_thread_component> mThreadComponents;
    backend::detail::thread_association mThreadAssociation;
};
}
