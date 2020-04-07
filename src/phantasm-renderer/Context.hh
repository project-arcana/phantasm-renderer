#pragma once

#include <atomic>
#include <mutex>

#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg-lean.hh>

#include <phantasm-hardware-interface/fwd.hh>
#include <phantasm-hardware-interface/types.hh>
#include <phantasm-hardware-interface/window_handle.hh>

#include <dxc-wrapper/compiler.hh>

#include <phantasm-renderer/common/gpu_epoch_tracker.hh>
#include <phantasm-renderer/common/multi_cache.hh>
#include <phantasm-renderer/common/single_cache.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fwd.hh>

namespace pr
{
/**
 * The context is the central manager of all graphics resources including the backend
 *
 *
 */
class Context
{
public:
    //
    // creation API
    //

    [[nodiscard]] raii::Frame make_frame(size_t initial_size = 2048);

    [[nodiscard]] auto_texture make_image(int width, format format, unsigned num_mips = 0, bool allow_uav = false);
    [[nodiscard]] auto_texture make_image(tg::isize2 size, format format, unsigned num_mips = 0, bool allow_uav = false);
    [[nodiscard]] auto_texture make_image(tg::isize3 size, format format, unsigned num_mips = 0, bool allow_uav = false);

    [[nodiscard]] auto_render_target make_target(tg::isize2 size, format format, unsigned num_samples = 1);

    [[nodiscard]] auto_buffer make_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    [[nodiscard]] auto_buffer make_upload_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);

    /// create a shader from binary data
    [[nodiscard]] auto_shader_binary make_shader(std::byte const* data, size_t size, phi::shader_stage stage);
    /// create a shader by compiling it live from text
    [[nodiscard]] auto_shader_binary make_shader(cc::string_view code, cc::string_view entrypoint, phi::shader_stage stage);

    [[nodiscard]] auto_prebuilt_argument make_graphics_argument(pr::argument const& arg);
    [[nodiscard]] auto_prebuilt_argument make_compute_argument(pr::argument const& arg);
    [[nodiscard]] argument_builder build_argument() { return {this}; }

    /// create a graphics pipeline state
    [[nodiscard]] auto_graphics_pipeline_state make_pipeline_state(graphics_pass_info const& gp, framebuffer_info const& fb);
    /// create a compute pipeline state
    [[nodiscard]] auto_compute_pipeline_state make_pipeline_state(compute_pass_info const& cp);

    //
    // cache lookup API
    //

    [[nodiscard]] cached_render_target get_target(tg::isize2 size, format format, unsigned num_samples = 1);

    [[nodiscard]] cached_buffer get_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    [[nodiscard]] cached_buffer get_upload_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);


    //
    // freeing API
    //

    /// free a resource that was unlocked from automatic management
    void free(raw_resource const& resource);

    /// free a buffer
    void free(buffer const& buffer) { free(buffer.res); }
    /// free a texture
    void free(texture const& texture) { free(texture.res); }
    /// free a render_target
    void free(render_target const& rt) { free(rt.res); }

    void free(graphics_pipeline_state const& pso) { freePipelineState(pso._handle); }
    void free(compute_pipeline_state const& pso) { freePipelineState(pso._handle); }
    void free(prebuilt_argument const& arg) { freeShaderView(arg._sv); }
    void free(shader_binary const& shader)
    {
        if (shader._owning_blob != nullptr)
            freeShaderBinary(shader._owning_blob);
    }

    /// free a buffer by placing it in the cache for reuse
    void free_to_cache(buffer const& buffer);
    /// free a texture by placing it in the cache for reuse
    void free_to_cache(texture const& texture);
    /// free a render target by placing it in the cache for reuse
    void free_to_cache(render_target const& rt);

    //
    // map upload API
    //

    void write_buffer(buffer const& buffer, void const* data, size_t size, size_t offset = 0);

    template <class T>
    void write_buffer_t(buffer const& buffer, T const& data)
    {
        static_assert(!std::is_pointer_v<T>, "[pr::Context::write_buffer_t] Pointer instead of raw data provided");
        static_assert(std::is_trivially_copyable_v<T>, "[pr::Context::write_buffer_t] Non-trivially copyable data provided");
        write_buffer(buffer, &data, sizeof(T));
    }

    //
    // consumption API
    //

    /// compiles a frame (records the command list)
    /// heavy operation, try to thread this if possible
    [[nodiscard]] CompiledFrame compile(raii::Frame& frame);

    /// submits a previously compiled frame to the GPU
    /// returns an epoch that can be waited on using Context::wait_for_epoch()
    gpu_epoch_t submit(CompiledFrame&& frame);

    /// convenience to compile and submit a frame in a single call
    /// returns an epoch that can be waited on using Context::wait_for_epoch()
    gpu_epoch_t submit(raii::Frame& frame);

    /// discard a previously compiled frame
    void discard(CompiledFrame&& frame);

    /// flips backbuffers
    void present();

    /// blocks on the CPU until all pending GPU operations are done
    void flush();

    /// conditional flush
    /// returns false if the epoch was reached already, or flushes and returns true
    bool flush(gpu_epoch_t epoch);

    void on_window_resize(tg::isize2 size);
    [[nodiscard]] bool clear_backbuffer_resize();
    tg::isize2 get_backbuffer_size() const;
    format get_backbuffer_format() const;

    [[nodiscard]] render_target acquire_backbuffer();

    bool start_capture();
    bool end_capture();

    phi::Backend& get_backend() { return *mBackend; }

public:
    //
    // ctors, init and destroy
    //

    Context() = default;
    /// constructs a context with a default backend (usually vulkan)
    Context(phi::window_handle const& window_handle, backend_type type = backend_type::vulkan);
    /// constructs a context with a specified backend
    Context(phi::Backend* backend);

    ~Context() { destroy(); }

    void initialize(phi::window_handle const& window_handle, backend_type type = backend_type::vulkan);
    void initialize(phi::Backend* backend);

    void destroy();

public:
    // deleted overrides

    // auto_ types are not to be freed manually, only free unlocked types
    template <class T, bool Cached>
    void free(auto_destroyer<T, Cached> const&) = delete;

    // auto_ types are not to be freed manually, only free unlocked types
    template <class T, bool Cached>
    void free_to_cache(auto_destroyer<T, Cached> const&) = delete;

    // ref type
private:
    Context(Context const&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context const&) = delete;
    Context& operator=(Context&&) = delete;

    // internal
private:
    [[nodiscard]] uint64_t acquireGuid();

private:
    void internalInitialize();

    // creation
    render_target createRenderTarget(render_target_info const& info);
    texture createTexture(texture_info const& info);
    buffer createBuffer(buffer_info const& info);

    // multi cache acquire
    render_target acquireRenderTarget(render_target_info const& info);
    texture acquireTexture(texture_info const& info);
    buffer acquireBuffer(buffer_info const& info);

    // internal RAII auto_destroyer API
private:
    friend struct detail::auto_destroy_proxy;

    void freeShaderBinary(IDxcBlob* blob);
    void freeShaderView(phi::handle::shader_view sv);
    void freePipelineState(phi::handle::pipeline_state ps);

    void freeCachedTarget(render_target_info const& info, raw_resource res);
    void freeCachedTexture(texture_info const& info, raw_resource res);
    void freeCachedBuffer(buffer_info const& info, raw_resource res);

    // internal builder API
private:
    friend struct pipeline_builder;
    friend struct argument_builder;

    // internal argument builder API
private:
    // single cache access, Frame-side API
private:
    friend class raii::Frame;
    phi::handle::pipeline_state acquire_graphics_pso(murmur_hash hash, const graphics_pass_info& gp, const framebuffer_info& fb);
    phi::handle::pipeline_state acquire_compute_pso(murmur_hash hash, const compute_pass_info& cp);

    phi::handle::shader_view acquire_graphics_sv(murmur_hash hash, hashable_storage<shader_view_info> const& info_storage);
    phi::handle::shader_view acquire_compute_sv(murmur_hash hash, hashable_storage<shader_view_info> const& info_storage);

    void free_all(cc::span<freeable_cached_obj const> freeables);

    void free_graphics_pso(murmur_hash hash);
    void free_compute_pso(murmur_hash hash);
    void free_graphics_sv(murmur_hash hash);
    void free_compute_sv(murmur_hash hash);

    // members
private:
    phi::Backend* mBackend = nullptr;
    bool mOwnsBackend = false;
    phi::sc::compiler mShaderCompiler;
    std::mutex mMutexSubmission;
    std::mutex mMutexShaderCompilation;

    // components
    gpu_epoch_tracker mGpuEpochTracker;
    std::atomic<uint64_t> mResourceGUID = {0};

    // caches (have dtors, members must be below backend ptr)
    multi_cache<render_target_info> mCacheRenderTargets;
    multi_cache<texture_info> mCacheTextures;
    multi_cache<buffer_info> mCacheBuffers;

    // single caches (no dtors)
    single_cache<phi::handle::pipeline_state> mCacheGraphicsPSOs;
    single_cache<phi::handle::pipeline_state> mCacheComputePSOs;
    single_cache<phi::handle::shader_view> mCacheGraphicsSVs;
    single_cache<phi::handle::shader_view> mCacheComputeSVs;
};
}
