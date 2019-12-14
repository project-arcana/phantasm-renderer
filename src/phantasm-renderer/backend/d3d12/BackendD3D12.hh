#pragma once

#include <phantasm-renderer/backend/Backend.hh>
#include <phantasm-renderer/backend/types.hh>

#include <phantasm-renderer/backend/detail/thread_association.hh>

#include "Adapter.hh"
#include "Device.hh"
#include "Queue.hh"
#include "Swapchain.hh"

#include "pools/cmd_list_pool.hh"
#include "pools/pso_pool.hh"
#include "pools/resource_pool.hh"
#include "pools/shader_view_pool.hh"

namespace pr::backend::device
{
class Window;
};

namespace pr::backend::d3d12
{
class BackendD3D12 final : public Backend
{
public:
    void initialize(backend_config const& config, device::Window& window) override;
    ~BackendD3D12() override;

public:
    // Virtual interface

    void flushGPU() override;

    //
    // Swapchain interface
    //

    [[nodiscard]] handle::resource acquireBackbuffer() override;
    void present() override { mSwapchain.present(); }
    void onResize(tg::isize2 size) override;
    [[nodiscard]] tg::isize2 getBackbufferSize() const override { return mSwapchain.getBackbufferSize(); }
    [[nodiscard]] format getBackbufferFormat() const override;


    //
    // Resource interface
    //

    [[nodiscard]] handle::resource createTexture(backend::format format, unsigned w, unsigned h, unsigned mips, texture_dimension dim, unsigned depth_or_array_size, bool allow_uav) override
    {
        return mPoolResources.createTexture(format, w, h, mips, dim, depth_or_array_size, allow_uav);
    }

    [[nodiscard]] handle::resource createRenderTarget(backend::format format, unsigned w, unsigned h, unsigned samples) override
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

    [[nodiscard]] std::byte* getMappedMemory(handle::resource res) override { return mPoolResources.getMappedMemory(res); }

    void free(handle::resource res) override { mPoolResources.free(res); }

    void free_range(cc::span<handle::resource const> resources) override { mPoolResources.free(resources); }


    //
    // Shader view interface
    //

    [[nodiscard]] handle::shader_view createShaderView(cc::span<shader_view_element const> srvs,
                                                       cc::span<shader_view_element const> uavs,
                                                       cc::span<sampler_config const> samplers,
                                                       bool /*usage_compute*/) override
    {
        return mPoolShaderViews.create(srvs, uavs, samplers);
    }

    void free(handle::shader_view sv) override { mPoolShaderViews.free(sv); }

    void free_range(cc::span<handle::shader_view const> svs) override { mPoolShaderViews.free(svs); }

    //
    // Pipeline state interface
    //

    [[nodiscard]] handle::pipeline_state createPipelineState(arg::vertex_format vertex_format,
                                                             arg::framebuffer_format framebuffer_format,
                                                             arg::shader_argument_shapes shader_arg_shapes,
                                                             arg::shader_stages shader_stages,
                                                             pr::primitive_pipeline_config const& primitive_config) override
    {
        return mPoolPSOs.createPipelineState(vertex_format, framebuffer_format, shader_arg_shapes, shader_stages, primitive_config);
    }

    [[nodiscard]] handle::pipeline_state createComputePipelineState(arg::shader_argument_shapes shader_arg_shapes, arg::shader_stage const& compute_shader) override
    {
        return mPoolPSOs.createComputePipelineState(shader_arg_shapes, compute_shader);
    }

    void free(handle::pipeline_state ps) override { mPoolPSOs.free(ps); }

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

    [[nodiscard]] ID3D12Device& getDevice() { return mDevice.getDevice(); }
    [[nodiscard]] ID3D12CommandQueue& getDirectQueue() { return mGraphicsQueue.getQueue(); }


private:
    // Core components
    Adapter mAdapter;
    Device mDevice;
    Queue mGraphicsQueue;
    Swapchain mSwapchain;

    // Pools
    ResourcePool mPoolResources;
    CommandListPool mPoolCmdLists;
    PipelineStateObjectPool mPoolPSOs;
    ShaderViewPool mPoolShaderViews;

    // Logic
    struct per_thread_component;
    cc::array<per_thread_component> mThreadComponents;
    backend::detail::thread_association mThreadAssociation;

private:
};
}
