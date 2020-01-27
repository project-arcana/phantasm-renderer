#pragma once

#include <atomic>
#include <mutex>

#include <clean-core/poly_unique_ptr.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg-lean.hh>

#include <phantasm-hardware-interface/types.hh>
#include <phantasm-hardware-interface/window_handle.hh>

#include <phantasm-renderer/Buffer.hh>
#include <phantasm-renderer/FragmentShader.hh>
#include <phantasm-renderer/Image.hh>
#include <phantasm-renderer/VertexShader.hh>
#include <phantasm-renderer/common/gpu_epoch_tracker.hh>
#include <phantasm-renderer/common/multi_cache.hh>
#include <phantasm-renderer/common/resource_info.hh>
#include <phantasm-renderer/fragment_type.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/vertex_type.hh>

namespace pr
{
/**
 * The context is the central manager of all graphics resources including the backend
 *
 *
 */
class Context
{
    // creation API
public:
    Frame make_frame(size_t initial_size = 2048);

    template <format F>
    Image<1, F> make_image(int width);
    template <format F>
    Image<2, F> make_image(tg::isize2 size);
    template <format F>
    Image<3, F> make_image(tg::isize3 size);

    template <format F, class T>
    Image<2, F> make_target(tg::isize2 size, T clear_val);

    template <class T, cc::enable_if<std::is_trivially_copyable_v<T>> = true>
    Buffer<T> make_upload_buffer(T const& data, bool read_only = true);
    template <class T>
    Buffer<T[]> make_upload_buffer(cc::span<T const> data, bool read_only = true);

    Buffer<untyped_tag> make_untyped_buffer(size_t size, size_t stride = 0, bool read_only = true);
    Buffer<untyped_tag> make_untyped_upload_buffer(size_t size, size_t stride = 0, bool read_only = true);

    template <format FragmentF>
    FragmentShader<FragmentF> make_fragment_shader(cc::string_view code)
    {
        auto const binary = compileShader(code, phi::shader_stage::pixel);
        return {binary};
    }

    template <format FragmentF>
    FragmentShader<FragmentF> make_fragment_shader(std::byte* binary, size_t size)
    {
        return {phi::arg::shader_binary{binary, size}};
    }

    template <class... VertexT>
    VertexShader<vertex_type_of<VertexT...>> make_vertex_shader(cc::string_view code)
    {
        auto const binary = compileShader(code, phi::shader_stage::vertex);
        return {binary};
    }

    template <class... VertexT>
    VertexShader<vertex_type_of<VertexT...>> make_vertex_shader(std::byte* binary, size_t size)
    {
        return {phi::arg::shader_binary{binary, size}};
    }

    // consumption API
public:
    [[nodiscard]] CompiledFrame compile(Frame const& frame);

    void submit(Frame const& frame);
    void submit(CompiledFrame const& frame);

    // ctors
public:
    /// constructs a context with a default backend (usually vulkan)
    Context(phi::window_handle const& window_handle);
    /// constructs a context with a specified backend
    Context(cc::poly_unique_ptr<phi::Backend> backend);

    ~Context();

    // ref type
public:
    Context(Context const&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context const&) = delete;
    Context& operator=(Context&&) = delete;

    // internal
public:
    [[nodiscard]] uint64_t acquireGuid() { return mResourceGUID.fetch_add(1); }

    void freeRenderTarget(render_target_info const& info, phi::handle::resource res, uint64_t guid);
    void freeTexture(texture_info const& info, phi::handle::resource res, uint64_t guid);
    void freeBuffer(buffer_info const& info, phi::handle::resource res, uint64_t guid);

private:
    void initialize();

    // creation
    phi::arg::shader_binary compileShader(cc::string_view code, phi::shader_stage stage);

    // multi cache acquire
    phi::handle::resource acquireRenderTarget(render_target_info const& info);
    phi::handle::resource acquireTexture(texture_info const& info);
    phi::handle::resource acquireBuffer(buffer_info const& info);

public:
    // single cache acquire
    phi::handle::pipeline_state acquirePSO(phi::arg::vertex_format const& vertex_fmt,
                                           phi::arg::framebuffer_config const& fb_conf,
                                           phi::arg::shader_arg_shapes arg_shapes,
                                           bool has_root_consts,
                                           phi::arg::graphics_shaders shaders,
                                           phi::pipeline_config const& pipeline_conf);

    // members
private:
    cc::poly_unique_ptr<phi::Backend> mBackend;
    std::mutex mMutexSubmission;

    // components
    gpu_epoch_tracker mGpuEpochTracker;
    std::atomic<uint64_t> mResourceGUID = {0};

    // caches
    multi_cache<render_target_info> mCacheRenderTargets;
    multi_cache<texture_info> mCacheTextures;
    multi_cache<buffer_info> mCacheBuffers;
};

// ============================== IMPLEMENTATION ==============================

template <format F>
Image<1, F> Context::make_image(int width)
{
    auto const info = texture_info{F, phi::texture_dimension::t1d, true, width, 1, 1, 0};
    return {this, acquireGuid(), acquireTexture(info), info};
}

template <format F>
Image<2, F> Context::make_image(tg::isize2 size)
{
    auto const info = texture_info{F, phi::texture_dimension::t2d, true, size.width, size.height, 1, 0};
    return {this, acquireGuid(), acquireTexture(info), info};
}

template <format F>
Image<3, F> Context::make_image(tg::isize3 size)
{
    auto const info = texture_info{F, phi::texture_dimension::t3d, true, size.width, size.height, unsigned(size.depth), 0};
    return {this, acquireGuid(), acquireTexture(info), info};
}

template <format F, class T>
Image<2, F> Context::make_target(tg::isize2 size, T /*clear_val*/) // TODO: clear value
{
    auto const info = render_target_info{F, size.width, size.height, 1};
    return {this, acquireGuid(), acquireRenderTarget(info), info};
}

template <class T>
Buffer<T[]> Context::make_upload_buffer(cc::span<const T> data, bool read_only)
{
    auto const info = buffer_info{sizeof(T) * data.size(), sizeof(T), !read_only, true};
    auto const handle = acquireBuffer(info);
    auto* const map = mBackend->getMappedMemory(handle);
    std::memcpy(map, data.data(), info.size_bytes);

    return {this, acquireGuid(), handle, info, map};
}

template <class T, cc::enable_if<std::is_trivially_copyable_v<T>>>
Buffer<T> Context::make_upload_buffer(const T& data, bool read_only)
{
    auto const info = buffer_info{sizeof(T), 0, !read_only, true};
    auto const handle = acquireBuffer(info);
    auto* const map = mBackend->getMappedMemory(handle);
    std::memcpy(map, &data, info.size_bytes);

    return {this, acquireGuid(), handle, info, map};
}

}
