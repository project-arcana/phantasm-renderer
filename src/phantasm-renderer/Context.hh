#pragma once
#include <cstddef>

#include <clean-core/fwd.hh>

#include <typed-geometry/types/size.hh>

#include <phantasm-hardware-interface/fwd.hh>

#include <phantasm-renderer/common/api.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/resource_types.hh>

namespace pr
{
/**
 * The context is the central manager of all graphics resources including the backend
 *
 *
 */
class PR_API Context
{
public:
    //
    // resource creation
    //

    /// start a frame, allowing command recording
    [[nodiscard]] raii::Frame make_frame(size_t initial_size = 2048, cc::allocator* alloc = cc::system_allocator);

    //
    // textures

    /// create a 1D texture
    [[nodiscard]] auto_texture make_texture(int32_t width, format format, uint32_t num_mips = 0, bool allow_uav = false, char const* debug_name = nullptr);

    /// create a 2D texture
    [[nodiscard]] auto_texture make_texture(tg::isize2 size, format format, uint32_t num_mips = 0, bool allow_uav = false, char const* debug_name = nullptr);

    /// create a 3D texture
    [[nodiscard]] auto_texture make_texture(tg::isize3 size, format format, uint32_t num_mips = 0, bool allow_uav = false, char const* debug_name = nullptr);

    /// create a texture cube
    [[nodiscard]] auto_texture make_texture_cube(tg::isize2 size, format format, uint32_t num_mips = 0, bool allow_uav = false, char const* debug_name = nullptr);

    /// create a 1D texture array
    [[nodiscard]] auto_texture make_texture_array(int32_t width, uint32_t num_elems, format format, uint32_t num_mips = 0, bool allow_uav = false, char const* debug_name = nullptr);

    /// create a 2D texture array
    [[nodiscard]] auto_texture make_texture_array(
        tg::isize2 size, uint32_t num_elems, format format, uint32_t num_mips = 0, bool allow_uav = false, char const* debug_name = nullptr);

    /// create a texture from an info struct
    [[nodiscard]] auto_texture make_texture(texture_info const& info, char const* debug_name = nullptr);

    /// create a texture from the info of a different texture. NOTE: does not concern contents or state
    [[nodiscard]] auto_texture make_texture_clone(texture const& clone_source, char const* debug_name = nullptr);

    //
    // render targets

    /// create a render target
    [[nodiscard]] auto_texture make_target(tg::isize2 size, format format, uint32_t num_samples = 1, uint32_t array_size = 1, char const* debug_name = nullptr);

    /// create a render target with an optimized clear value
    [[nodiscard]] auto_texture make_target(
        tg::isize2 size, format format, uint32_t num_samples, uint32_t array_size, phi::rt_clear_value optimized_clear, char const* debug_name = nullptr);

    //
    // buffers

    /// create a buffer
    [[nodiscard]] auto_buffer make_buffer(uint32_t size, uint32_t stride = 0, bool allow_uav = false, char const* debug_name = nullptr);

    /// create a mapped upload buffer which can be directly written to from CPU
    [[nodiscard]] auto_buffer make_upload_buffer(uint32_t size, uint32_t stride = 0, char const* debug_name = nullptr);

    /// create a mapped upload buffer, with a size based on accomodating a given texture's contents
    [[nodiscard]] auto_buffer make_upload_buffer_for_texture(texture const& tex, uint32_t num_mips = 1, char const* debug_name = nullptr);

    /// create a mapped readback buffer which can be directly read from CPU
    [[nodiscard]] auto_buffer make_readback_buffer(uint32_t size, uint32_t stride = 0, char const* debug_name = nullptr);

    /// create a buffer from an info struct
    [[nodiscard]] auto_buffer make_buffer(buffer_info const& info, char const* debug_name = nullptr);

    /// create a buffer from the info of a different buffer. NOTE: does not concern contents or state
    [[nodiscard]] auto_buffer make_buffer_clone(buffer const& clone_source, char const* debug_name = nullptr);

    //
    // shaders

    /// create a shader from binary data (only hashes the data)
    [[nodiscard]] auto_shader_binary make_shader(cc::span<std::byte const> data, pr::shader stage);

    /// create a shader by compiling it live from text
    /// build_debug: compile without optimizations and embed debug symbols/PDB info (/Od /Zi /Qembed_debug) - required for shader debugging in Rdoc, PIX etc
    [[nodiscard]] auto_shader_binary make_shader(
        cc::string_view code, cc::string_view entrypoint, pr::shader stage, bool build_debug = false, cc::allocator* scratch_alloc = cc::system_allocator);

    //
    // prebuilt arguments (shader views)

    /// create a persisted shader argument for graphics passes
    [[nodiscard]] auto_prebuilt_argument make_graphics_argument(pr::argument& arg);

    /// create a persisted shader argument for graphics passes from raw phi types
    [[nodiscard]] auto_prebuilt_argument make_graphics_argument(cc::span<phi::resource_view const> srvs,
                                                                cc::span<phi::resource_view const> uavs,
                                                                cc::span<phi::sampler_config const> samplers);

    /// create a persisted shader argument for compute passes
    [[nodiscard]] auto_prebuilt_argument make_compute_argument(pr::argument& arg);

    /// create a persisted shader argument for compute passes from raw phi types
    [[nodiscard]] auto_prebuilt_argument make_compute_argument(cc::span<phi::resource_view const> srvs,
                                                               cc::span<phi::resource_view const> uavs,
                                                               cc::span<phi::sampler_config const> samplers);

    /// create a persisted shader argument, builder pattern
    [[nodiscard]] argument_builder build_argument(cc::allocator* temp_alloc = cc::system_allocator);

    //
    // pipeline states

    /// create a graphics pipeline state
    [[nodiscard]] auto_graphics_pipeline_state make_pipeline_state(graphics_pass_info const& gp, framebuffer_info const& fb);

    /// create a compute pipeline state
    [[nodiscard]] auto_compute_pipeline_state make_pipeline_state(compute_pass_info const& cp);

    //
    // query ranges

    /// create a contiguous range of queries
    [[nodiscard]] auto_query_range make_query_range(pr::query_type type, uint32_t num_queries);

    //
    // resource cache lookup
    //

    /// create or retrieve a render target from the cache
    [[nodiscard]] cached_texture get_target(tg::isize2 size, format format, uint32_t num_samples = 1, uint32_t array_size = 1);

    /// create or retrieve a render target with an optimized clear value from the cache
    [[nodiscard]] cached_texture get_target(tg::isize2 size, format format, uint32_t num_samples, uint32_t array_size, phi::rt_clear_value optimized_clear);

    /// create or retrieve a buffer from the cache
    [[nodiscard]] cached_buffer get_buffer(uint32_t size, uint32_t stride = 0, bool allow_uav = false);
    [[nodiscard]] cached_buffer get_upload_buffer(uint32_t size, uint32_t stride = 0);
    [[nodiscard]] cached_buffer get_readback_buffer(uint32_t size, uint32_t stride = 0);
    [[nodiscard]] cached_buffer get_buffer(buffer_info const& info);

    /// create or retrieve a texture from the cache
    [[nodiscard]] cached_texture get_texture(texture_info const& info);

    //
    // untyped resource creation and -cache lookup
    //

    /// create a resource of undetermined type, without RAII management (use free_untyped)
    [[nodiscard]] resource make_untyped_unlocked(generic_resource_info const& info, char const* debug_name = nullptr);

    /// create or retrieve a resource of undetermined type from cache, without RAII management (use free_to_cache_untyped)
    [[nodiscard]] resource get_untyped_unlocked(generic_resource_info const& info);

    //
    // freeing
    //   destroy a resource immediately
    //   must not be CPU-used after call, or referenced in future Frame submits
    //   must not be in flight on GPU during call
    //

    /// free a resource that was disowned from automatic management
    void free_untyped(phi::handle::resource resource);

    void free_untyped(resource const& resource) { free_untyped(resource.handle); }

    /// free a buffer
    void free(buffer const& buffer) { free_untyped(buffer.handle); }

    /// free a texture
    void free(texture const& texture) { free_untyped(texture.handle); }

    void free(graphics_pipeline_state const& pso);
    void free(compute_pipeline_state const& pso);
    void free(prebuilt_argument const& arg);
    void free(shader_binary const& shader);
    void free(query_range const& q);

    void free_range(cc::span<phi::handle::resource const> res_range);
    void free_range(cc::span<prebuilt_argument const> arg_range);
    void free_range(cc::span<phi::handle::shader_view const> sv_range);

    /// convenience to free multiple (disowned) resources
    template <class... Res>
    void free_multiple_resources(Res&&... res_args)
    {
        // any resource still wrapped in the auto_ wrapper wouldn't have a .handle member (but instead .data.handle)
        phi::handle::resource flat_handles[] = {res_args.handle...};
        free_range(flat_handles);
    }

    //
    // cache freeing
    //   place a resource back into the cache for reuse
    //   must not be CPU-used after call, or referenced in future Frame submits
    //

    /// free a resource of undetermined type by placing it in the cache for reuse
    void free_to_cache_untyped(resource const& resource);

    /// free a buffer by placing it in the cache for reuse
    void free_to_cache(buffer const& buffer);

    /// free a texture by placing it in the cache for reuse
    void free_to_cache(texture const& texture);

    //
    // deferred freeing
    //   destroy a resource in the future once currently pending GPU operations are done
    //   must not be CPU-used after call, or referenced in future Frame submits
    //   use Frame::free_deferred_after_submit if the latter can't be guaranteed
    //

    /// free a buffer once no longer in flight
    void free_deferred(buffer const& buf);

    /// free a texture once no longer in flight
    void free_deferred(texture const& tex);

    /// free a resource once no longer in flight
    void free_deferred(resource const& res);

    /// free a PSO once no longer in flight
    void free_deferred(graphics_pipeline_state const& gpso);
    void free_deferred(compute_pipeline_state const& cpso);

    /// free raw PHI resources once no longer in flight
    void free_deferred(phi::handle::resource res);
    void free_deferred(phi::handle::shader_view sv);
    void free_deferred(phi::handle::pipeline_state pso);
    void free_range_deferred(cc::span<phi::handle::resource const> res_range);
    void free_range_deferred(cc::span<phi::handle::shader_view const> sv_range);

    //
    // buffer map API
    //

    /// map a buffer created on the upload or readback heap in order to access it on CPU
    /// a buffer can be mapped multiple times at once
    /// invalidate_begin and -end specify the range of CPU-side read data in bytes, end == -1 being the entire width
    /// NOTE: begin > 0 does not add an offset to the returned pointer
    [[nodiscard]] std::byte* map_buffer(buffer const& buffer, int32_t invalidate_begin = 0, int32_t invalidate_end = -1);

    /// map a buffer and return a span of the mapped memory (instead of just a pointer)
    [[nodiscard]] cc::span<std::byte> map_buffer_as_span(buffer const& buffer, int32_t invalidate_begin = 0, int32_t invalidate_end = -1);

    /// unmap a buffer previously mapped using map_buffer
    /// a buffer can be destroyed while mapped
    /// on non-desktop it might be required to unmap upload buffers for the writes to become visible
    /// begin and end specify the range of CPU-side modified data in bytes, end == -1 being the entire width
    void unmap_buffer(buffer const& buffer, int32_t flush_begin = 0, int32_t flush_end = -1);

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

    /// create a fence for synchronisation, initial value 0
    [[nodiscard]] auto_fence make_fence();

    void free(fence const& f);

    /// signal a fence to a given value from CPU
    void signal_fence_cpu(fence const& fence, uint64_t new_value);

    /// block on CPU until a fence reaches a given value
    void wait_fence_cpu(fence const& fence, uint64_t wait_value);

    /// read the current value of a fence
    [[nodiscard]] uint64_t get_fence_value(fence const& fence);

    //
    // command list submission
    //

    /// compiles a frame (records the command list)
    /// heavy operation, try to thread this if possible
    [[nodiscard]] CompiledFrame compile(raii::Frame&& frame);

    /// submits a previously compiled frame to the GPU
    /// returns an epoch that can be tested using Context::is_gpu_epoch_reached
    gpu_epoch_t submit(CompiledFrame&& frame);

    /// convenience to compile and submit a frame in a single call
    /// returns an epoch that can be tested using Context::is_gpu_epoch_reached
    gpu_epoch_t submit(raii::Frame&& frame);

    /// discard a previously compiled frame
    void discard(CompiledFrame&& frame);

    //
    // swapchain API
    //

    /// create a swapchain on a given window
    [[nodiscard]] auto_swapchain make_swapchain(phi::window_handle const& window_handle,
                                                tg::isize2 initial_size,
                                                pr::present_mode mode = pr::present_mode::synced,
                                                uint32_t num_backbuffers = 3);

    /// destroy a swapchain
    void free(swapchain const& sc);

    /// flips backbuffers on the given swapchain,
    /// showing the previously acquired backbuffer on screen
    /// NOTE: this requires a previous call to acquire_backbuffer
    void present(swapchain const& sc);

    /// resize a swapchain's backbuffers based on a window (OS) resize event
    /// NOTE: backbuffers won't always resize, and can have a different size than the window
    void on_window_resize(swapchain const& sc, tg::isize2 size);

    /// returns true if the swapchain's backbuffers were resized since the last call
    [[nodiscard]] bool clear_backbuffer_resize(swapchain const& sc);

    /// acquires the next backbuffer in line
    [[nodiscard]] texture acquire_backbuffer(swapchain const& sc);

    /// returns the size of the swapchain's backbuffers
    tg::isize2 get_backbuffer_size(swapchain const& sc) const;

    /// returns the format of the swapchain's backbuffers
    /// (on D3D12 and Desktop Vulkan this is always format::bgra8un)
    format get_backbuffer_format(swapchain const& sc) const;

    /// returns the amount of backbuffers a swapchain contains
    uint32_t get_num_backbuffers(swapchain const& sc) const;

    //
    // GPU synchronization
    //

    /// blocks on the CPU until all pending GPU operations are done
    void flush();

    /// returns whether the epoch was reached on the GPU
    bool is_gpu_epoch_reached(gpu_epoch_t epoch) const;

    /// "shuts down" the context: calls flush(), no longer accepts frame submits, and
    /// no longer asserts on non-disowned resources getting destroyed
    /// NOTE: this NOT the same as Context::destroy()
    void flush_and_shutdown();

    //
    // cache management
    //

    /// frees all resources (textures, rendertargets, buffers) from pr caches that are not acquired or in flight
    /// returns amount of freed elements
    uint32_t clear_resource_caches();

    /// frees all shader_views from pr caches that are not acquired or in flight
    /// returns amount of freed elements
    uint32_t clear_shader_view_cache();

    /// frees all pipeline_states from pr caches that are not acquired or in flight
    /// returns amount of freed elements
    uint32_t clear_pipeline_state_cache();

    /// runs all deferred free operations that are no longer in flight
    /// this also happens automatically on any call to free_deferred
    /// returns amount of freed elements
    uint32_t clear_pending_deferred_frees();

    //
    // phi interop
    //

    /// resource must have an assigned GUID if they are meant to interoperate with pr's caching mechanisms
    /// use this method to "import" a raw phi resource
    [[nodiscard]] resource import_phi_resource(phi::handle::resource raw_resource) { return {raw_resource}; }

    [[nodiscard]] buffer import_phi_buffer(phi::handle::resource raw_resource) { return {import_phi_resource(raw_resource)}; }

    [[nodiscard]] texture import_phi_texture(phi::handle::resource raw_resource) { return {import_phi_resource(raw_resource)}; }

    //
    // general info
    //

    texture_info const& get_texture_info(pr::texture const& tex) const;

    buffer_info const& get_buffer_info(pr::buffer const& buf) const;

    uint64_t get_gpu_timestamp_frequency() const;

    /// returns the difference between two GPU timestamp values in milliseconds
    double get_timestamp_difference_milliseconds(uint64_t start, uint64_t end) const
    {
        return (double(end - start) / get_gpu_timestamp_frequency()) * 1000.;
    }
    /// returns the difference between two GPU timestamp values in microseconds
    uint64_t get_timestamp_difference_microseconds(uint64_t start, uint64_t end) const
    {
        return (end - start) / (get_gpu_timestamp_frequency() / 1'000'000);
    }

    /// returns the amount of bytes needed to store the contents of a texture (in a GPU buffer)
    /// ex. use case: allocating upload buffers of the right size to upload textures
    uint32_t calculate_texture_upload_size(tg::isize3 size, format fmt, uint32_t num_mips = 1) const;
    uint32_t calculate_texture_upload_size(tg::isize2 size, format fmt, uint32_t num_mips = 1) const;
    uint32_t calculate_texture_upload_size(int32_t width, format fmt, uint32_t num_mips = 1) const;
    uint32_t calculate_texture_upload_size(texture const& texture, uint32_t num_mips = 1) const;

    /// returns the offset in bytes of the given pixel position in a texture of given size and format (in a GPU buffer)
    /// ex. use case: copying a render target to a readback buffer, then reading the pixel at this offset
    uint32_t calculate_texture_pixel_offset(tg::isize2 size, format fmt, tg::ivec2 pixel) const;

    bool is_shutting_down() const;

    //
    // miscellaneous
    //

    /// resets the debug name of a resource
    /// this is the name visible to diagnostic tools and referred to by validation warnings
    void set_debug_name(texture const& tex, cc::string_view name);
    void set_debug_name(buffer const& buf, cc::string_view name);
    void set_debug_name(phi::handle::resource raw_res, cc::string_view name);

    /// attempts to start a capture in a connected tool like Renderdoc, PIX, NSight etc
    bool start_capture();

    /// ends a capture previously started with start_capture()
    bool stop_capture();

    /// returns the underlying phantasm-hardware-interface backend
    phi::Backend& get_backend() { return *mBackend; }
    pr::backend get_backend_type() const { return mBackendType; }

    /// uint64 incremented on every submit, always greater or equal to GPU
    gpu_epoch_t get_current_cpu_epoch() const;

    /// uint64 incremented after every finished commandlist, GPU timeline, always less or equal to CPU
    gpu_epoch_t get_current_gpu_epoch() const;

public:
    //
    // ctors, init and destroy
    //

    Context() = default;

    /// internally create a backend with default config
    explicit Context(backend type, cc::allocator* alloc = cc::system_allocator) { initialize(type, alloc); }

    /// internally create a backend with specified config
    explicit Context(backend type, phi::backend_config const& config, cc::allocator* alloc = cc::system_allocator)
    {
        initialize(type, config, alloc);
    }

    /// attach to an existing backend
    explicit Context(phi::Backend* backend, cc::allocator* alloc = cc::system_allocator) { initialize(backend, alloc); }

    ~Context() { destroy(); }

    /// initializes the context
    void initialize(backend type, cc::allocator* alloc = cc::system_allocator);
    void initialize(backend type, phi::backend_config const& config, cc::allocator* alloc = cc::system_allocator);

    /// initializes the context with a pre-existing PHI backend
    void initialize(phi::Backend* backend, cc::allocator* alloc = cc::system_allocator);

    /// destroys the context
    void destroy();

    bool is_initialized() const { return mBackend != nullptr; }

public:
    // deleted overrides

    // auto_ types are not to be freed manually, only free disowned types
    template <class T, auto_mode M>
    [[deprecated("auto_ types must not be explicitly freed, use .disown() for manual management")]] void free(auto_destroyer<T, M> const&) = delete;

    // auto_ types are not to be freed manually, only free disowned types
    template <class T, auto_mode M>
    [[deprecated("auto_ types must not be explicitly freed, use .disown() for manual management")]] void free_to_cache(auto_destroyer<T, M> const&) = delete;

    // auto_ types are not to be freed manually, only free disowned types
    template <class T, auto_mode M>
    [[deprecated("auto_ types must not be explicitly freed, use .disown() for manual management")]] void free_deferred(auto_destroyer<T, M> const&) = delete;

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

    void internalInitialize(cc::allocator* alloc, bool ownsBackend);

    // creation
    texture createTexture(texture_info const& info, char const* dbg_name = nullptr);
    buffer createBuffer(buffer_info const& info, char const* dbg_name = nullptr);

    // multi cache acquire
    texture acquireTexture(texture_info const& info);
    buffer acquireBuffer(buffer_info const& info);

    // internal RAII auto_destroyer API
private:
    friend struct detail::auto_destroy_proxy;

    void freeShaderBinary(IDxcBlob* blob);
    void freeShaderView(phi::handle::shader_view sv);
    void freePipelineState(phi::handle::pipeline_state ps);

    void freeCachedTexture(texture_info const& info, resource res);
    void freeCachedBuffer(buffer_info const& info, resource res);

    // single cache access, Frame-side API
private:
    friend class raii::Frame;
    phi::handle::pipeline_state acquire_graphics_pso(uint64_t hash, const graphics_pass_info& gp, const framebuffer_info& fb);
    phi::handle::pipeline_state acquire_compute_pso(uint64_t hash, const compute_pass_info& cp);

    phi::handle::shader_view acquire_graphics_sv(uint64_t hash, hashable_storage<shader_view_info> const& info_storage);
    phi::handle::shader_view acquire_compute_sv(uint64_t hash, hashable_storage<shader_view_info> const& info_storage);

    void free_all(cc::span<freeable_cached_obj const> freeables);

    void free_graphics_pso(uint64_t hash);
    void free_compute_pso(uint64_t hash);
    void free_graphics_sv(uint64_t hash);
    void free_compute_sv(uint64_t hash);

    // members
private:
    phi::Backend* mBackend = nullptr;
    pr::backend mBackendType;

    struct Implementation;
    Implementation* mImpl = nullptr;
};
}
