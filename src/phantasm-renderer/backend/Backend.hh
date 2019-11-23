#pragma once

#include <typed-geometry/tg-lean.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/primitive_pipeline_config.hh>

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

    //
    // Swapchain interface
    //

    [[nodiscard]] virtual handle::resource acquireBackbuffer() = 0;
    [[nodiscard]] virtual tg::ivec2 getBackbufferSize() = 0;
    virtual void resize(int w, int h) = 0;
    virtual void present() = 0;

    //
    // Resource interface
    //

    /// create a 2D texture2
    [[nodiscard]] virtual handle::resource createTexture2D(backend::format format, int w, int h, int mips) = 0;

    /// create a render- or depth-stencil target
    [[nodiscard]] virtual handle::resource createRenderTarget(backend::format format, int w, int h) = 0;

    /// create a buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] virtual handle::resource createBuffer(unsigned size_bytes, resource_state initial_state, unsigned stride_bytes = 0) = 0;

    /// create a mapped, UPLOAD_HEAP buffer, with an element stride if its an index or vertex buffer
    [[nodiscard]] virtual handle::resource createMappedBuffer(unsigned size_bytes, unsigned stride_bytes = 0) = 0;

    virtual void free(handle::resource res) = 0;

    [[nodiscard]] virtual std::byte* getMappedMemory(handle::resource res) = 0;

    //
    // Shader view interface
    //

    [[nodiscard]] virtual handle::shader_view createShaderView(cc::span<handle::resource> srvs, cc::span<handle::resource> uavs = {}) = 0;

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

    virtual void submit(handle::command_list cl) = 0;

protected:
    Backend() = default;

private:
};
}
