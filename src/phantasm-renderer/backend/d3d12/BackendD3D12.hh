#pragma once

#include <phantasm-renderer/backend/Backend.hh>
#include <phantasm-renderer/backend/types.hh>

#include "Adapter.hh"
#include "Device.hh"
#include "Queue.hh"
#include "Swapchain.hh"
#include "cmd_list_translation.hh"

#include "pools/cmd_list_pool.hh"
#include "pools/pso_pool.hh"
#include "pools/resource_pool.hh"
#include "pools/shader_view_pool.hh"

namespace pr::backend::d3d12
{
class BackendD3D12 final : public Backend
{
public:
    void initialize(backend_config const& config, HWND handle);
    void destroy();

    /// flush all pending work on the GPU
    void flushGPU();

public:
    //
    // Swapchain interface
    //

    [[nodiscard]] handle::resource acquireBackbuffer();
    [[nodiscard]] tg::ivec2 getBackbufferSize() { return mSwapchain.getBackbufferSize(); }
    void resize(int w, int h) { mSwapchain.onResize(w, h); }
    void present() { mSwapchain.present(); }

    //
    // Resource interface
    //

    /// create a 2D texture2
    [[nodiscard]] handle::resource createTexture2D(backend::format format, int w, int h, int mips)
    {
        return mPoolResources.createTexture2D(format, w, h, mips);
    }

    /// create a render- or depth-stencil target
    [[nodiscard]] handle::resource createRenderTarget(backend::format format, int w, int h)
    {
        return mPoolResources.createRenderTarget(format, w, h);
    }

    /// create a buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createBuffer(unsigned size_bytes, resource_state initial_state, unsigned stride_bytes = 0)
    {
        return mPoolResources.createBuffer(size_bytes, initial_state, stride_bytes);
    }

    /// create a mapped, UPLOAD_HEAP buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] handle::resource createMappedBuffer(unsigned size_bytes, unsigned stride_bytes = 0)
    {
        return mPoolResources.createMappedBuffer(size_bytes, stride_bytes);
    }

    void free(handle::resource res) { mPoolResources.free(res); }

    [[nodiscard]] std::byte* getMappedMemory(handle::resource res) { return mPoolResources.getMappedMemory(res); }

    //
    // Shader view interface
    //

    [[nodiscard]] handle::shader_view createShaderView(cc::span<handle::resource> srvs, cc::span<handle::resource> uavs = {})
    {
        return mPoolShaderViews.create(srvs, uavs);
    }

    void free(handle::shader_view sv) { mPoolShaderViews.free(sv); }

    //
    // Pipeline state interface
    //

    [[nodiscard]] handle::pipeline_state createPipelineState(arg::vertex_format vertex_format,
                                                             arg::framebuffer_format framebuffer_format,
                                                             arg::shader_argument_shapes shader_arg_shapes,
                                                             arg::shader_stages shader_stages,
                                                             pr::primitive_pipeline_config const& primitive_config)
    {
        return mPoolPSOs.createPipelineState(vertex_format, framebuffer_format, shader_arg_shapes, shader_stages, primitive_config);
    }

    void free(handle::pipeline_state ps) { mPoolPSOs.free(ps); }

    //
    // Command list interface
    //

    [[nodiscard]] handle::command_list recordCommandList(std::byte* buffer, size_t size);

    void discard(handle::command_list cl) { mPoolCmdLists.freeOnDiscard(cl); }

    void submit(handle::command_list cl);

public:
    // backend-internal

    [[nodiscard]] ID3D12Device& getDevice() { return mDevice.getDevice(); }
    [[nodiscard]] ID3D12CommandQueue& getDirectQueue() { return mDirectQueue.getQueue(); }

private:
    // Core components
    Adapter mAdapter;
    Device mDevice;
    Queue mDirectQueue;
    Swapchain mSwapchain;

    // Pools
    ResourcePool mPoolResources;
    CommandListPool mPoolCmdLists;
    PipelineStateObjectPool mPoolPSOs;
    ShaderViewPool mPoolShaderViews;

    // Logic
    command_list_translator mTranslator;

private:
};
}
