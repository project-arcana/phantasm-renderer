#pragma once

#include <typed-geometry/types/size.hh>

#include <phantasm-renderer/fwd.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/fwd.hh>

namespace pr::backend
{
class Backend
{
    // reference type
public:
    Backend(Backend const&) = delete;
    Backend(Backend&&) = delete;
    Backend& operator=(Backend const&) = delete;
    Backend& operator=(Backend&&) = delete;
    virtual ~Backend() = default;

    virtual void initialize(backend_config const& config, device::Window& window) = 0;

    virtual void flushGPU() = 0;

    //
    // Swapchain interface
    //

    /// acquires a resource handle for use as a render target
    /// if the returned handle is handle::null_resource, the current frame must be discarded
    /// can cause an internal resize
    [[nodiscard]] virtual handle::resource acquireBackbuffer() = 0;

    /// attempts to present,
    /// can fail and cause an internal resize
    virtual void present() = 0;

    /// causes an internal resize
    virtual void onResize(tg::isize2 size) = 0;

    /// gets the current backbuffer size
    [[nodiscard]] virtual tg::isize2 getBackbufferSize() const = 0;

    /// gets the backbuffer pixel format
    [[nodiscard]] virtual format getBackbufferFormat() const = 0;

    /// Clears pending internal resize events, returns true if the
    /// backbuffer has resized since the last call
    [[nodiscard]] bool clearPendingResize()
    {
        if (mHasResized)
        {
            mHasResized = false;
            return true;
        }
        else
        {
            return false;
        }
    }

    //
    // Resource interface
    //

    /// create a 2D texture2
    [[nodiscard]] virtual handle::resource createTexture2D(backend::format format, int w, int h, int mips) = 0;

    /// create a render- or depth-stencil target
    [[nodiscard]] virtual handle::resource createRenderTarget(backend::format format, int w, int h, int samples = 1) = 0;

    /// create a buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] virtual handle::resource createBuffer(unsigned size_bytes, resource_state initial_state, unsigned stride_bytes = 0) = 0;

    /// create a mapped, UPLOAD_HEAP buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] virtual handle::resource createMappedBuffer(unsigned size_bytes, unsigned stride_bytes = 0) = 0;

    virtual void free(handle::resource res) = 0;

    [[nodiscard]] virtual std::byte* getMappedMemory(handle::resource res) = 0;

    //
    // Shader view interface
    //

    [[nodiscard]] virtual handle::shader_view createShaderView(cc::span<shader_view_element const> srvs,
                                                               cc::span<shader_view_element const> uavs,
                                                               cc::span<sampler_config const> samplers)
        = 0;

    virtual void free(handle::shader_view sv) = 0;

    //
    // Pipeline state interface
    //

    [[nodiscard]] virtual handle::pipeline_state createPipelineState(arg::vertex_format vertex_format,
                                                                     arg::framebuffer_format framebuffer_format,
                                                                     arg::shader_argument_shapes shader_arg_shapes,
                                                                     arg::shader_stages shader_stages,
                                                                     pr::primitive_pipeline_config const& primitive_config)
        = 0;

    virtual void free(handle::pipeline_state ps) = 0;

    //
    // Command list interface
    //

    [[nodiscard]] virtual handle::command_list recordCommandList(std::byte* buffer, size_t size) = 0;

    virtual void discard(handle::command_list cl) = 0;

    virtual void submit(cc::span<handle::command_list const> cls) = 0;

    //
    // Debug interface
    //

    virtual void printInformation(handle::resource res) const = 0;

protected:
    Backend() = default;

    void onInternalResize() { mHasResized = true; }

private:
    bool mHasResized = true;
};
}
