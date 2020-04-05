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
    // creation API
public:
    [[nodiscard]] raii::Frame make_frame(size_t initial_size = 2048);

    [[nodiscard]] image make_image(int width, format format, unsigned num_mips = 0, bool allow_uav = false);
    [[nodiscard]] image make_image(tg::isize2 size, format format, unsigned num_mips = 0, bool allow_uav = false);
    [[nodiscard]] image make_image(tg::isize3 size, format format, unsigned num_mips = 0, bool allow_uav = false);

    [[nodiscard]] render_target make_target(tg::isize2 size, format format, unsigned num_samples = 1);

    [[nodiscard]] buffer make_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    [[nodiscard]] buffer make_upload_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);

    /// create a shader from binary data
    [[nodiscard]] shader_binary make_shader(std::byte const* data, size_t size, phi::shader_stage stage);
    /// create a shader by compiling it live from text
    [[nodiscard]] shader_binary make_shader(cc::string_view code, cc::string_view entrypoint, phi::shader_stage stage);

    [[nodiscard]] argument_builder build_argument() { return {this}; }

    /// create a graphics pipeline state
    [[nodiscard]] graphics_pipeline_state make_pipeline_state(graphics_pass_info const& gp, framebuffer_info const& fb);
    /// create a compute pipeline state
    [[nodiscard]] compute_pipeline_state make_pipeline_state(compute_pass_info const& cp);

    // map upload API
public:
    void write_buffer(buffer const& buffer, void const* data, size_t size, size_t offset = 0);

    template <class T>
    void write_buffer_t(buffer const& buffer, T const& data)
    {
        static_assert(!std::is_pointer_v<T>, "[pr::Context::write_buffer_t] Pointer instead of raw data provided");
        static_assert(std::is_trivially_copyable_v<T>, "[pr::Context::write_buffer_t] Non-trivially copyable data provided");
        write_buffer(buffer, &data, sizeof(T));
    }

    // cache lookup API
public:
    [[nodiscard]] cached_render_target get_target(tg::isize2 size, format format, unsigned num_samples = 1);

    [[nodiscard]] cached_buffer get_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    [[nodiscard]] cached_buffer get_upload_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);

    // consumption API
public:
    [[nodiscard]] CompiledFrame compile(raii::Frame& frame);

    void submit(raii::Frame& frame);
    void submit(CompiledFrame&& frame);
    void discard(CompiledFrame&& frame);

    void present();
    void flush();

    void on_window_resize(tg::isize2 size);
    [[nodiscard]] bool clear_backbuffer_resize();
    tg::isize2 get_backbuffer_size() const;
    format get_backbuffer_format() const;

    [[nodiscard]] render_target acquire_backbuffer();

    bool start_capture();
    bool end_capture();

    // ctors
public:
    /// constructs a context with a default backend (usually vulkan)
    Context(phi::window_handle const& window_handle);
    /// constructs a context with a specified backend
    Context(phi::Backend* backend);

    ~Context();

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
    void initialize();

    // creation
    pr::resource createRenderTarget(render_target_info const& info);
    pr::resource createTexture(texture_info const& info);
    pr::resource createBuffer(buffer_info const& info);

    // multi cache acquire
    pr::resource acquireRenderTarget(render_target_info const& info);
    pr::resource acquireTexture(texture_info const& info);
    pr::resource acquireBuffer(buffer_info const& info);

    // internal RAII dtor API
private:
    friend struct resource_data;
    friend struct shader_binary_data;
    friend struct prebuilt_argument_data;
    friend struct pipeline_state_abstract;
    void freeResource(phi::handle::resource res);
    void freeShaderBinary(IDxcBlob* blob);
    void freeShaderView(phi::handle::shader_view sv);
    void freePipelineState(phi::handle::pipeline_state ps);

    friend struct buffer;
    friend struct render_target;
    void freeCachedTarget(render_target_info const& info, resource&& res);
    void freeCachedTexture(texture_info const& info, pr::resource&& res);
    void freeCachedBuffer(buffer_info const& info, pr::resource&& res);

    // internal builder API
private:
    friend struct pipeline_builder;
    friend struct argument_builder;
    phi::Backend* get_backend() const { return mBackend; }

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
    phi::Backend* mBackend;
    bool const mOwnsBackend;
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
