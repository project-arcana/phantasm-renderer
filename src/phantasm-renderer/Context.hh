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
#include <phantasm-renderer/fwd.hh>

#include <phantasm-renderer/argument.hh>
#include <phantasm-renderer/reflection/vertex_attributes.hh>
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
    Frame make_frame(size_t initial_size = 2048);

    image make_image(int width, format format, unsigned num_mips = 0, bool allow_uav = false);
    image make_image(tg::isize2 size, format format, unsigned num_mips = 0, bool allow_uav = false);
    image make_image(tg::isize3 size, format format, unsigned num_mips = 0, bool allow_uav = false);

    render_target make_target(tg::isize2 size, format format, unsigned num_samples = 1);

    buffer make_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);
    buffer make_upload_buffer(unsigned size, unsigned stride = 0, bool allow_uav = false);

    shader_binary make_shader(cc::string_view code, cc::string_view entrypoint, phi::shader_stage stage);

    baked_shader_view make_argument(shader_view const& arg, bool usage_compute = false);

    template <class VertT, class... ShaderTs>
    graphics_pipeline_state make_graphics_pipeline_state(phi::arg::framebuffer_config const& framebuf_config,
                                                         phi::arg::shader_arg_shapes arg_shapes,
                                                         bool has_root_constants,
                                                         phi::pipeline_config const& config,
                                                         ShaderTs const&... shader_binaries)
    {
        cc::capped_vector<phi::arg::graphics_shader, 6> shaders;

        auto const add_shader = [&](shader_binary const& binary) {
            shaders.push_back(phi::arg::graphics_shader{{binary.data._data, binary.data._size}, binary.data._stage});
        };
        (add_shader(shader_binaries), ...);
        return make_graphics_pipeline_state(phi::arg::vertex_format{get_vertex_attributes<VertT>(), sizeof(VertT)}, framebuf_config, arg_shapes,
                                            has_root_constants, shaders, config);
    }

    graphics_pipeline_state make_graphics_pipeline_state(phi::arg::vertex_format const& vert_format,
                                                         phi::arg::framebuffer_config const& framebuf_config,
                                                         phi::arg::shader_arg_shapes arg_shapes,
                                                         bool has_root_consts,
                                                         phi::arg::graphics_shaders shader,
                                                         const phi::pipeline_config& config = {});

    compute_pipeline_state make_compute_pipeline_state(phi::arg::shader_arg_shapes arg_shapes, bool has_root_constants, shader_binary const& shader);

    // map upload API
public:
    void write_buffer(buffer const& buffer, void const* data, size_t size);

    template <class T>
    void write_buffer_t(buffer const& buffer, T const& data)
    {
        static_assert(!std::is_pointer_v<T>, "[pr::Context::write_buffer_t] Pointer instead of raw data provided");
        static_assert(std::is_trivially_copyable_v<T>, "[pr::Context::write_buffer_t] Non-trivially copyable data provided");
        write_buffer(buffer, &data, sizeof(T));
    }

    // cache lookup API
public:
    cached_render_target get_target(tg::isize2 size, format format, unsigned num_samples = 1);

    cached_buffer get_buffer(size_t size, size_t stride = 0, bool allow_uav = false);
    cached_buffer get_upload_buffer(size_t size, size_t stride = 0, bool allow_uav = false);

    cached_shader_binary get_shader(cc::string_view code, phi::shader_stage stage);

    // consumption API
public:
    [[nodiscard]] CompiledFrame compile(Frame const& frame);

    void submit(Frame const& frame);
    void submit(CompiledFrame const& frame);

    void present();
    void flush();

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
    friend struct baked_shader_view_data;
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

    // caches
    multi_cache<render_target_info> mCacheRenderTargets;
    multi_cache<texture_info> mCacheTextures;
    multi_cache<buffer_info> mCacheBuffers;
};
}
