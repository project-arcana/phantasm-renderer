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
#include <phantasm-renderer/enums.hh>
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

    /// start a frame, allowing command recording
    [[nodiscard]] raii::Frame make_frame(size_t initial_size = 2048);

    /// create a 1D texture
    [[nodiscard]] auto_texture make_texture(int width, format format, unsigned num_mips = 0, bool allow_uav = false);
    /// create a 2D texture
    [[nodiscard]] auto_texture make_texture(tg::isize2 size, format format, unsigned num_mips = 0, bool allow_uav = false);
    /// create a 3D texture
    [[nodiscard]] auto_texture make_texture(tg::isize3 size, format format, unsigned num_mips = 0, bool allow_uav = false);
    /// create a texture cube
    [[nodiscard]] auto_texture make_texture_cube(tg::isize2 size, format format, unsigned num_mips = 0, bool allow_uav = false);
    /// create a 1D texture array
    [[nodiscard]] auto_texture make_texture_array(int width, unsigned num_elems, format format, unsigned num_mips = 0, bool allow_uav = false);
    /// create a 2D texture array
    [[nodiscard]] auto_texture make_texture_array(tg::isize2 size, unsigned num_elems, format format, unsigned num_mips = 0, bool allow_uav = false);

    /// create a render target
    [[nodiscard]] auto_render_target make_target(tg::isize2 size, format format, unsigned num_samples = 1, unsigned array_size = 1);
    /// create a render target with an optimized clear value
    [[nodiscard]] auto_render_target make_target(tg::isize2 size, format format, unsigned num_samples, unsigned array_size, phi::rt_clear_value const& optimized_clear);

    /// create a buffer
    [[nodiscard]] auto_buffer make_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    /// create a mapped upload buffer which can be directly written to from CPU
    [[nodiscard]] auto_buffer make_upload_buffer(unsigned size, unsigned stride = 0);
    /// create a mapped upload buffer, with a size based on accomodating a given texture's contents
    [[nodiscard]] auto_buffer make_upload_buffer_for_texture(texture const& tex, unsigned num_mips = 1);
    /// create a mapped readback buffer which can be directly read from CPU
    [[nodiscard]] auto_buffer make_readback_buffer(unsigned size, unsigned stride = 0);

    /// create a shader from binary data
    [[nodiscard]] auto_shader_binary make_shader(std::byte const* data, size_t size, pr::shader stage);
    /// create a shader by compiling it live from text
    [[nodiscard]] auto_shader_binary make_shader(cc::string_view code, cc::string_view entrypoint, pr::shader stage);

    /// create a persisted shader argument for graphics passes
    [[nodiscard]] auto_prebuilt_argument make_graphics_argument(pr::argument const& arg);
    /// create a persisted shader argument for compute passes
    [[nodiscard]] auto_prebuilt_argument make_compute_argument(pr::argument const& arg);
    /// create a persisted shader argument, builder pattern
    [[nodiscard]] argument_builder build_argument() { return {this}; }

    /// create a graphics pipeline state
    [[nodiscard]] auto_graphics_pipeline_state make_pipeline_state(graphics_pass_info const& gp, framebuffer_info const& fb);
    /// create a compute pipeline state
    [[nodiscard]] auto_compute_pipeline_state make_pipeline_state(compute_pass_info const& cp);

    //
    // cache lookup API
    //

    [[nodiscard]] cached_render_target get_target(tg::isize2 size, format format, unsigned num_samples = 1, unsigned array_size = 1);

    [[nodiscard]] cached_buffer get_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    [[nodiscard]] cached_buffer get_upload_buffer(unsigned size, unsigned stride = 0);
    [[nodiscard]] cached_buffer get_readback_buffer(unsigned size, unsigned stride = 0);


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

    void flush_buffer_writes(buffer const& buffer);

    [[nodiscard]] std::byte* get_buffer_map(buffer const& buffer);

    //
    // consumption API
    //

    /// compiles a frame (records the command list)
    /// heavy operation, try to thread this if possible
    [[nodiscard]] CompiledFrame compile(raii::Frame&& frame);

    /// submits a previously compiled frame to the GPU
    /// returns an epoch that can be waited on using Context::wait_for_epoch()
    gpu_epoch_t submit(CompiledFrame&& frame);

    /// convenience to compile and submit a frame in a single call
    /// returns an epoch that can be waited on using Context::wait_for_epoch()
    gpu_epoch_t submit(raii::Frame&& frame);

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

    [[nodiscard]] render_target acquire_backbuffer();

    //
    // cache management
    //

    /// frees all resources (textures, rendertargets, buffers) from pr caches that are not acquired or in flight
    /// returns amount of freed elements
    unsigned clear_resource_caches();

    /// frees all shader_views from pr caches that are not acquired or in flight
    /// returns amount of freed elements
    unsigned clear_shader_view_cache();

    /// frees all pipeline_states from pr caches that are not acquired or in flight
    /// returns amount of freed elements
    unsigned clear_pipeline_state_cache();

    //
    // info
    //

    tg::isize2 get_backbuffer_size() const;
    format get_backbuffer_format() const;
    unsigned get_num_backbuffers() const;

    /// returns the amount of bytes needed to store the contents of a texture in an upload buffer
    unsigned calculate_texture_upload_size(tg::isize3 size, format fmt, unsigned num_mips = 1) const;
    unsigned calculate_texture_upload_size(tg::isize2 size, format fmt, unsigned num_mips = 1) const;
    unsigned calculate_texture_upload_size(int width, format fmt, unsigned num_mips = 1) const;
    unsigned calculate_texture_upload_size(texture const& texture, unsigned num_mips = 1) const;

    //
    // miscellaneous
    //

    bool start_capture();
    bool end_capture();

    phi::Backend& get_backend() { return *mBackend; }

public:
    //
    // ctors, init and destroy
    //

    Context() = default;
    /// internally create a backend with default config
    Context(phi::window_handle const& window_handle, backend type = backend::vulkan);
    /// internally create a backend with specified config
    Context(phi::window_handle const& window_handle, backend type, phi::backend_config const& config);
    /// attach to an existing backend
    Context(phi::Backend* backend);

    ~Context() { destroy(); }

    void initialize(phi::window_handle const& window_handle, backend type = backend::vulkan);
    void initialize(phi::window_handle const& window_handle, backend type, phi::backend_config const& config);
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
    render_target createRenderTarget(render_target_info const& info, phi::rt_clear_value const* optimized_clear = nullptr);
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
    std::atomic<uint64_t> mResourceGUID = {1}; // GUID 0 is invalid

    // caches (have dtors, members must be below backend ptr)
    multi_cache<render_target_info> mCacheRenderTargets;
    multi_cache<texture_info> mCacheTextures;
    multi_cache<buffer_info> mCacheBuffers;

    // single caches (no dtors)
    single_cache<phi::handle::pipeline_state> mCacheGraphicsPSOs;
    single_cache<phi::handle::pipeline_state> mCacheComputePSOs;
    single_cache<phi::handle::shader_view> mCacheGraphicsSVs;
    single_cache<phi::handle::shader_view> mCacheComputeSVs;

    // safety/assert state
#ifdef CC_ENABLE_ASSERTIONS
    struct
    {
        bool did_acquire_before_present = false;
    } mSafetyState;
#endif
};

inline auto_buffer Context::make_upload_buffer_for_texture(const texture& tex, unsigned num_mips)
{
    return make_upload_buffer(calculate_texture_upload_size(tex, num_mips));
}

inline unsigned Context::calculate_texture_upload_size(tg::isize2 size, phi::format fmt, unsigned num_mips) const
{
    return calculate_texture_upload_size({size.width, size.height, 1}, fmt, num_mips);
}

inline unsigned Context::calculate_texture_upload_size(int width, phi::format fmt, unsigned num_mips) const
{
    return calculate_texture_upload_size({width, 1, 1}, fmt, num_mips);
}

inline unsigned Context::calculate_texture_upload_size(const texture& texture, unsigned num_mips) const
{
    return calculate_texture_upload_size({texture.info.width, texture.info.height, int(texture.info.depth_or_array_size)}, texture.info.fmt, num_mips);
}
}
