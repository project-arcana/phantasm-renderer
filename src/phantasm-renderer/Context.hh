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
    /// create a texture from an info struct
    [[nodiscard]] auto_texture make_texture(texture_info const& info);
    /// create a texture from the info of a different texture. NOTE: does not concern contents or state
    [[nodiscard]] auto_texture make_texture_clone(texture const& clone_source) { return make_texture(clone_source.info); }

    /// create a render target
    [[nodiscard]] auto_render_target make_target(tg::isize2 size, format format, unsigned num_samples = 1, unsigned array_size = 1);
    /// create a render target with an optimized clear value
    [[nodiscard]] auto_render_target make_target(tg::isize2 size, format format, unsigned num_samples, unsigned array_size, phi::rt_clear_value optimized_clear);
    /// create a render target from an info struct
    [[nodiscard]] auto_render_target make_target(render_target_info const& info);
    /// create a render target from the info of a different one. NOTE: does not concern contents or state
    [[nodiscard]] auto_render_target make_target_clone(render_target const& clone_source) { return make_target(clone_source.info); }

    /// create a buffer
    [[nodiscard]] auto_buffer make_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    /// create a mapped upload buffer which can be directly written to from CPU
    [[nodiscard]] auto_buffer make_upload_buffer(unsigned size, unsigned stride = 0);
    /// create a mapped upload buffer, with a size based on accomodating a given texture's contents
    [[nodiscard]] auto_buffer make_upload_buffer_for_texture(texture const& tex, unsigned num_mips = 1);
    /// create a mapped readback buffer which can be directly read from CPU
    [[nodiscard]] auto_buffer make_readback_buffer(unsigned size, unsigned stride = 0);
    /// create a buffer from an info struct
    [[nodiscard]] auto_buffer make_buffer(buffer_info const& info);
    /// create a buffer from the info of a different buffer. NOTE: does not concern contents or state
    [[nodiscard]] auto_buffer make_buffer_clone(buffer const& clone_source) { return make_buffer(clone_source.info); }


    /// create a shader from binary data
    [[nodiscard]] auto_shader_binary make_shader(cc::span<cc::byte const> data, pr::shader stage);
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

    /// create a fence for synchronisation, initial value 0
    [[nodiscard]] auto_fence make_fence();
    /// create a contiguous range of queries
    [[nodiscard]] auto_query_range make_query_range(pr::query_type type, unsigned num_queries);

    //
    // cache lookup API
    //

    /// create or retrieve a render target from the cache
    [[nodiscard]] cached_render_target get_target(tg::isize2 size, format format, unsigned num_samples = 1, unsigned array_size = 1);
    /// create or retrieve a render target with an optimized clear value from the cache
    [[nodiscard]] cached_render_target get_target(tg::isize2 size, format format, unsigned num_samples, unsigned array_size, phi::rt_clear_value optimized_clear);
    /// create or retrieve a render target from an info struct from the cache
    [[nodiscard]] cached_render_target get_target(render_target_info const& info);

    /// create or retrieve a buffer from the cache
    [[nodiscard]] cached_buffer get_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    [[nodiscard]] cached_buffer get_upload_buffer(unsigned size, unsigned stride = 0);
    [[nodiscard]] cached_buffer get_readback_buffer(unsigned size, unsigned stride = 0);
    [[nodiscard]] cached_buffer get_buffer(buffer_info const& info);

    /// create or retrieve a texture from the cache
    [[nodiscard]] cached_texture get_texture(texture_info const& info);


    //
    // untyped creation and cache lookup API
    //

    /// create a resource of undetermined type, without RAII management (use free_untyped)
    [[nodiscard]] raw_resource make_untyped_unlocked(generic_resource_info const& info);

    /// create or retrieve a resource of undetermined type from cache, without RAII management (use free_to_cache_untyped)
    [[nodiscard]] raw_resource get_untyped_unlocked(generic_resource_info const& info);

    //
    // freeing API
    //

    /// free a resource that was unlocked from automatic management
    void free_untyped(phi::handle::resource resource);
    void free_untyped(raw_resource const& resource) { free_untyped(resource.handle); }

    /// free a buffer
    void free(buffer const& buffer) { free_untyped(buffer.res); }
    /// free a texture
    void free(texture const& texture) { free_untyped(texture.res); }
    /// free a render_target
    void free(render_target const& rt) { free_untyped(rt.res); }

    void free(graphics_pipeline_state const& pso) { freePipelineState(pso._handle); }
    void free(compute_pipeline_state const& pso) { freePipelineState(pso._handle); }
    void free(prebuilt_argument const& arg) { freeShaderView(arg._sv); }
    void free(shader_binary const& shader)
    {
        if (shader._owning_blob != nullptr)
            freeShaderBinary(shader._owning_blob);
    }
    void free(fence const& f);
    void free(query_range const& q);

    /// free a resource of undetermined type by placing it in the cache for reuse
    void free_to_cache_untyped(raw_resource const& resource, generic_resource_info const& info);
    /// free a buffer by placing it in the cache for reuse
    void free_to_cache(buffer const& buffer);
    /// free a texture by placing it in the cache for reuse
    void free_to_cache(texture const& texture);
    /// free a render target by placing it in the cache for reuse
    void free_to_cache(render_target const& rt);

    //
    // buffer map API
    //

    /// map a buffer created on the upload or readback heap in order to access it on CPU
    /// a buffer can be mapped multiple times at once
    [[nodiscard]] std::byte* map_buffer(buffer const& buffer);

    /// unmap a buffer previously mapped using map_buffer
    /// a buffer can be destroyed while mapped
    void unmap_buffer(buffer const& buffer);

    /// map an upload buffer, memcpy the provided data into it, and unmap it
    void write_to_buffer_raw(buffer const& buffer, cc::span<std::byte const> data, size_t offset_in_buffer = 0);

    /// map a readback buffer, memcpy its contents to the provided location, and unmap it
    void read_from_buffer_raw(buffer const& buffer, cc::span<std::byte> out_data, size_t offset_in_buffer = 0);

    /// map an upload buffer, memcpy the provided data into it, and unmap it
    /// data can be POD, or a container with POD elements (ie. cc::vector<int>)
    template <class T>
    void write_to_buffer(buffer const& buffer, T const& data, size_t offset_in_buffer = 0)
    {
        static_assert(!std::is_pointer_v<T>, "[pr::Context::write_to_buffer_t] Pointer instead of raw data provided");
        write_to_buffer_raw(buffer, cc::as_byte_span(data), offset_in_buffer);
    }

    /// map a readback buffer, memcpy its contents to the provided location, and unmap it
    /// data can be POD, or a container with POD elements (ie. cc::vector<int>)
    template <class T>
    void read_from_buffer(buffer const& buffer, T& out_data, size_t offset_in_buffer = 0)
    {
        static_assert(!std::is_pointer_v<T>, "[pr::Context::read_from_buffer_t] Pointer instead of raw data provided");
        read_from_buffer_raw(buffer, cc::as_byte_span(out_data), offset_in_buffer);
    }

    //
    // fence API
    //

    /// signal a fence to a given value from CPU
    void signal_fence_cpu(fence const& fence, uint64_t new_value);
    /// block on CPU until a fence reaches a given value
    void wait_fence_cpu(fence const& fence, uint64_t wait_value);

    /// signal a fence to a given value from a specified GPU queue
    void signal_fence_gpu(fence const& fence, uint64_t new_value, queue_type queue);
    /// block on a specified GPU queue until a fence reaches a given value
    void wait_fence_gpu(fence const& fence, uint64_t wait_value, queue_type queue);

    /// read the current value of a fence
    [[nodiscard]] uint64_t get_fence_value(fence const& fence);

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
    [[deprecated("unimplemented")]] unsigned clear_shader_view_cache();

    /// frees all pipeline_states from pr caches that are not acquired or in flight
    /// returns amount of freed elements
    [[deprecated("unimplemented")]] unsigned clear_pipeline_state_cache();

    //
    // phi interop
    //

    /// resource must have an assigned GUID if they are meant to interoperate with pr's caching mechanisms
    /// use this method to "import" a raw phi resource
    [[nodiscard]] raw_resource import_phi_resource(phi::handle::resource raw_resource) { return {raw_resource, acquireGuid()}; }

    [[nodiscard]] buffer import_phi_buffer(phi::handle::resource raw_resource, buffer_info const& info)
    {
        return {import_phi_resource(raw_resource), info};
    }

    [[nodiscard]] render_target import_phi_target(phi::handle::resource raw_resource, render_target_info const& info)
    {
        return {import_phi_resource(raw_resource), info};
    }

    [[nodiscard]] texture import_phi_texture(phi::handle::resource raw_resource, texture_info const& info)
    {
        return {import_phi_resource(raw_resource), info};
    }

    //
    // info
    //

    tg::isize2 get_backbuffer_size() const;
    format get_backbuffer_format() const;
    unsigned get_num_backbuffers() const;
    uint64_t get_gpu_timestamp_frequency() const { return mGPUTimestampFrequency; }

    double get_timestamp_difference_milliseconds(uint64_t start, uint64_t end) const
    {
        return (double(end - start) / mGPUTimestampFrequency) * 1000.;
    }

    /// returns the amount of bytes needed to store the contents of a texture (in a GPU buffer)
    unsigned calculate_texture_upload_size(tg::isize3 size, format fmt, unsigned num_mips = 1) const;
    unsigned calculate_texture_upload_size(tg::isize2 size, format fmt, unsigned num_mips = 1) const;
    unsigned calculate_texture_upload_size(int width, format fmt, unsigned num_mips = 1) const;
    unsigned calculate_texture_upload_size(texture const& texture, unsigned num_mips = 1) const;

    /// returns the offset in bytes of the given pixel position in a texture of given size and format (in a GPU buffer)
    unsigned calculate_texture_pixel_offset(tg::isize2 size, format fmt, tg::ivec2 pixel) const;


    //
    // miscellaneous
    //

    /// attempts to start a capture in a connected tool like Renderdoc, PIX, NSight etc
    bool start_capture();
    /// ends a capture previously started with start_capture()
    bool stop_capture();

    /// returns the underlying phantasm-hardware-interface backend
    phi::Backend& get_backend() { return *mBackend; }
    pr::backend get_backend_type() const { return mBackendType; }

    /// monotonously increasing uint64, always greater or equal to GPU epoch
    gpu_epoch_t get_current_cpu_epoch() const { return mGpuEpochTracker.get_current_epoch_cpu(); }

    /// monotously increasing uint64, GPU timeline, always less or equal to CPU
    gpu_epoch_t get_current_gpu_epoch() const { return mGpuEpochTracker.get_current_epoch_gpu(); }

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

    bool is_initialized() const { return mBackend != nullptr; }

public:
    // deleted overrides

    // auto_ types are not to be freed manually, only free unlocked types
    template <class T, bool Cached>
    void free(auto_destroyer<T, Cached> const&) = delete;

    // auto_ types are not to be freed manually, only free unlocked types
    template <class T, bool Cached>
    void free_to_cache(auto_destroyer<T, Cached> const&) = delete;

private:
    //
    // internal from here on out
    //

private:
    Context(Context const&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context const&) = delete;
    Context& operator=(Context&&) = delete;

private:
    [[nodiscard]] uint64_t acquireGuid();

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

    // single cache access, Frame-side API
private:
    friend class raii::Frame;
    phi::handle::pipeline_state acquire_graphics_pso(cc::hash_t hash, const graphics_pass_info& gp, const framebuffer_info& fb);
    phi::handle::pipeline_state acquire_compute_pso(cc::hash_t hash, const compute_pass_info& cp);

    phi::handle::shader_view acquire_graphics_sv(cc::hash_t hash, hashable_storage<shader_view_info> const& info_storage);
    phi::handle::shader_view acquire_compute_sv(cc::hash_t hash, hashable_storage<shader_view_info> const& info_storage);

    void free_all(cc::span<freeable_cached_obj const> freeables);

    void free_graphics_pso(cc::hash_t hash);
    void free_compute_pso(cc::hash_t hash);
    void free_graphics_sv(cc::hash_t hash);
    void free_compute_sv(cc::hash_t hash);

    // members
private:
    // backend
    phi::Backend* mBackend = nullptr;
    bool mOwnsBackend = false;
    pr::backend mBackendType;

    // constant info
    uint64_t mGPUTimestampFrequency;

    // components
    dxcw::compiler mShaderCompiler;
    std::mutex mMutexSubmission;
    std::mutex mMutexShaderCompilation;
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

inline unsigned Context::calculate_texture_upload_size(tg::isize2 size, pr::format fmt, unsigned num_mips) const
{
    return calculate_texture_upload_size({size.width, size.height, 1}, fmt, num_mips);
}

inline unsigned Context::calculate_texture_upload_size(int width, pr::format fmt, unsigned num_mips) const
{
    return calculate_texture_upload_size({width, 1, 1}, fmt, num_mips);
}

inline unsigned Context::calculate_texture_upload_size(const texture& texture, unsigned num_mips) const
{
    return calculate_texture_upload_size({texture.info.width, texture.info.height, int(texture.info.depth_or_array_size)}, texture.info.fmt, num_mips);
}
}
