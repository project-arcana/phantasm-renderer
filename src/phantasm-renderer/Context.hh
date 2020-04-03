#pragma once

#include <atomic>
#include <mutex>

#include <clean-core/poly_unique_ptr.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg-lean.hh>

#include <phantasm-hardware-interface/types.hh>
#include <phantasm-hardware-interface/window_handle.hh>

#include <dxc-wrapper/compiler.hh>

#include <phantasm-renderer/common/gpu_epoch_tracker.hh>
#include <phantasm-renderer/common/multi_cache.hh>
#include <phantasm-renderer/common/resource_info.hh>
#include <phantasm-renderer/common/single_cache.hh>
#include <phantasm-renderer/fwd.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/pipeline_builder.hh>
#include <phantasm-renderer/resource_types.hh>

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
    [[nodiscard]] Frame make_frame(size_t initial_size = 2048);

    [[nodiscard]] image make_image(int width, format format, unsigned num_mips = 0, bool allow_uav = false);
    [[nodiscard]] image make_image(tg::isize2 size, format format, unsigned num_mips = 0, bool allow_uav = false);
    [[nodiscard]] image make_image(tg::isize3 size, format format, unsigned num_mips = 0, bool allow_uav = false);

    [[nodiscard]] render_target make_target(tg::isize2 size, format format, unsigned num_samples = 1);

    [[nodiscard]] buffer make_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    [[nodiscard]] buffer make_upload_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);

    [[nodiscard]] shader_binary make_shader(cc::string_view code, cc::string_view entrypoint, phi::shader_stage stage);

    [[nodiscard]] baked_argument make_argument(argument const& arg, bool usage_compute = false);

    [[nodiscard]] pipeline_builder build_pipeline_state() { return {this}; }

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

    [[nodiscard]] cached_shader_binary get_shader(cc::string_view code, phi::shader_stage stage);

    // consumption API
public:
    [[nodiscard]] CompiledFrame compile(Frame& frame);

    void submit(Frame& frame);
    void submit(CompiledFrame&& frame);

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
    Context(cc::poly_unique_ptr<phi::Backend>&& backend);

    ~Context();

    // ref type
private:
    Context(Context const&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context const&) = delete;
    Context& operator=(Context&&) = delete;

    // internal
private:
    [[nodiscard]] uint64_t acquireGuid() { return mResourceGUID.fetch_add(1); }

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
    friend struct baked_argument_data;
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

    // internal PSO builder API
private:
    friend struct pipeline_builder;
    compute_pipeline_state create_compute_pso(phi::arg::shader_arg_shapes arg_shapes, bool has_root_consts, phi::arg::shader_binary shader);
    graphics_pipeline_state create_graphics_pso(phi::arg::vertex_format const& vert_format,
                                                phi::arg::framebuffer_config const& framebuf_config,
                                                phi::arg::shader_arg_shapes arg_shapes,
                                                bool has_root_consts,
                                                phi::arg::graphics_shaders shader,
                                                const phi::pipeline_config& config);

private:
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
    phi::sc::compiler mShaderCompiler;
    std::mutex mMutexSubmission;

    // components
    gpu_epoch_tracker mGpuEpochTracker;
    std::atomic<uint64_t> mResourceGUID = {0};

    // caches (have dtors, members must be below backend ptr)
    multi_cache<render_target_info> mCacheRenderTargets;
    multi_cache<texture_info> mCacheTextures;
    multi_cache<buffer_info> mCacheBuffers;
};
}
