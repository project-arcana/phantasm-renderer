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
    void initialize(backend_config const& config, device::Window& window) override;
    ~BackendVulkan() override;

public:
    // Virtual interface

    //
    // Swapchain interface
    //

    [[nodiscard]] handle::resource acquireBackbuffer() override;
    void present() override;
    void onResize(tg::isize2 size) override;
    [[nodiscard]] tg::isize2 getBackbufferSize() const override { return mSwapchain.getBackbufferSize(); }
    [[nodiscard]] format getBackbufferFormat() const override;


    //
    // Resource interface
    //

    [[nodiscard]] handle::resource createTexture(backend::format format, int w, int h, int mips, texture_dimension dim, int depth_or_array_size) override
    {
        return mPoolResources.createTexture(format, w, h, mips, dim, depth_or_array_size);
    }

    [[nodiscard]] handle::resource createRenderTarget(backend::format format, int w, int h, int samples) override
    {
        return mPoolResources.createRenderTarget(format, w, h, samples);
    }

    [[nodiscard]] handle::resource createBuffer(unsigned size_bytes, resource_state initial_state, unsigned stride_bytes = 0) override
    {
        return mPoolResources.createBuffer(size_bytes, initial_state, stride_bytes);
    }

    [[nodiscard]] handle::resource createMappedBuffer(unsigned size_bytes, unsigned stride_bytes = 0) override
    {
        return mPoolResources.createMappedBuffer(size_bytes, stride_bytes);
    }

    void free(handle::resource res) override { mPoolResources.free(res); }

    [[nodiscard]] std::byte* getMappedMemory(handle::resource res) override { return mPoolResources.getMappedMemory(res); }

    //
    // Shader view interface
    //

    [[nodiscard]] handle::shader_view createShaderView(cc::span<shader_view_element const> srvs,
                                                       cc::span<shader_view_element const> uavs,
                                                       cc::span<sampler_config const> samplers) override
    {
        return mPoolShaderViews.create(srvs, uavs, samplers);
    }

    void free(handle::shader_view sv) override { mPoolShaderViews.free(sv); }

    //
    // Pipeline state interface
    //

    [[nodiscard]] handle::pipeline_state createPipelineState(arg::vertex_format vertex_format,
                                                             arg::framebuffer_format framebuffer_format,
                                                             arg::shader_argument_shapes shader_arg_shapes,
                                                             arg::shader_stages shader_stages,
                                                             pr::primitive_pipeline_config const& primitive_config) override
    {
        return mPoolPipelines.createPipelineState(vertex_format, framebuffer_format, shader_arg_shapes, shader_stages, primitive_config);
    }

    void free(handle::pipeline_state ps) override { mPoolPipelines.free(ps); }

    //
    // Command list interface
    //

    [[nodiscard]] handle::command_list recordCommandList(std::byte* buffer, size_t size) override;
    void discard(cc::span<handle::command_list const> cls) override { mPoolCmdLists.freeOnDiscard(cls); }
    void submit(cc::span<handle::command_list const> cls) override;

    //
    // Debug interface
    //

    void printInformation(handle::resource res) const override;

public:
    // backend-internal

    /// flush all pending work on the GPU
    void flushGPU() override;

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
