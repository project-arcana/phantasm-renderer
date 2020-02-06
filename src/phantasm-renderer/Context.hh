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

    shader_binary make_shader(cc::string_view code, phi::shader_stage stage);

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

    void freeRenderTarget(render_target_info const& info, pr::resource res);
    void freeTexture(texture_info const& info, pr::resource res);
    void freeBuffer(buffer_info const& info, pr::resource res);

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
