#pragma once

#include <clean-core/alloc_vector.hh>
#include <clean-core/defer.hh>
#include <clean-core/span.hh>

#include <typed-geometry/types/size.hh>

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/common/api.hh>
#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/enums.hh>
#include <phantasm-renderer/fwd.hh>

#include <phantasm-renderer/resource_types.hh>

#include "ComputePass.hh"
#include "Framebuffer.hh"

namespace pr::raii
{
class PR_API Frame
{
public:
    //
    // framebuffer RAII API

    // create a framebuffer by supplying render targets (all cleared, viewport minimum of sizes)
    template <class... RTs>
    [[nodiscard]] Framebuffer make_framebuffer(RTs const&... targets) &
    {
        static_assert(sizeof...(RTs) > 0, "at least one render target required");

        phi::cmd::begin_render_pass bcmd;
        // initialize command
        bcmd.set_null_depth_stencil();
        bcmd.viewport = tg::isize2(1 << 30, 1 << 30);

        int num_samples = -1;

        // add targets
        (addRenderTargetToFramebuffer(bcmd, num_samples, targets), ...);
        return buildFramebuffer(bcmd, num_samples, nullptr, true);
    }

    // create a framebuffer from a raw phi command
    [[nodiscard]] Framebuffer make_framebuffer(phi::cmd::begin_render_pass const& raw_command, bool auto_transition = true) &;

    // create a framebuffer using a builder with more configuration options
    [[nodiscard]] framebuffer_builder build_framebuffer() & { return {this}; }

    //
    // pass RAII API (compute only, graphics passes are in Framebuffer)

    // start a compute pass from persisted PSO
    [[nodiscard]] ComputePass make_pass(compute_pipeline_state const& compute_pipeline) & { return {this, compute_pipeline.handle}; }

    // start a compute pass from a raw phi PSO
    [[nodiscard]] ComputePass make_pass(phi::handle::pipeline_state raw_pso) & { return {this, raw_pso}; }

    // fetch a PSO from cache - this hits a OS mutex and might have to build a PSO (expensive)
    [[nodiscard]] ComputePass make_pass(compute_pass_info const& cp) &;

    //
    // transitions and present

    void transition(buffer const& res, state target, shader_flags dependency = {});
    void transition(texture const& res, state target, shader_flags dependency = {});
    void transition(phi::handle::resource raw_resource, state target, shader_flags dependency = {});

    // create a UAV barrier to synchronize GPU-GPU access
    // if no resources are specified, the barrier is global
    void barrier_uav(cc::span<phi::handle::resource const> resources);

    // transition the backbuffer to present state and trigger a Context::present after this frame is submitted
    void present_after_submit(texture const& backbuffer, swapchain sc);

    //
    // commands

    // copy buffer to buffer
    void copy(buffer const& src, buffer const& dest, uint32_t src_offset = 0, uint32_t dest_offset = 0, uint32_t num_bytes = 0);

    // copy buffer to texture
    // writes the specified MIP level of the specified array element
    void copy(buffer const& src, texture const& dest, uint32_t src_offset = 0, uint32_t dest_mip_index = 0, uint32_t dest_array_index = 0);

    // copy texture to buffer
    // reads an entire MIP level of a specified array element
    void copy(texture const& src, buffer const& dest, uint32_t dest_offset = 0, uint32_t src_mip_index = 0, uint32_t src_array_index = 0);

    // copy texture to buffer
    // reads the specified MIP level of the specified array element
    // only copies a specified section of the MIP, size given in texels
    void copy(texture const& src,
              buffer const& dest,
              tg::uvec3 src_offset_texels,
              tg::uvec3 src_extent_texels,
              uint32_t dest_offset = 0,
              uint32_t src_mip_index = 0,
              uint32_t src_array_index = 0);

    // copies all array slices at the given MIP level from src to dest
    void copy(texture const& src, texture const& dest, uint32_t mip_index = 0);

    // copy textures specifying source and destination MIP, array element, size and slice count
    void copy_subsection(texture const& src,
                         texture const& dest,
                         uint32_t src_mip_index,
                         uint32_t src_array_index,
                         uint32_t dest_mip_index,
                         uint32_t dest_array_index,
                         uint32_t num_array_slices,
                         tg::isize2 dest_size);

    // resolve a multisampled texture (to a regular one)
    void resolve(texture const& src, texture const& dest);

    // write a timestamp to a query in a given (timestamp) query range
    void write_timestamp(query_range const& query_range, uint32_t index);

    // resolve one or more queries in a range and write their contents to a buffer
    void resolve_queries(query_range const& src, buffer const& dest, uint32_t first_query, uint32_t num_queries, uint32_t dest_offset_bytes = 0);

    // begin a debug label region (visible in renderdoc, nsight, gpa, pix, etc.)
    void begin_debug_label(char const* label);
    void end_debug_label();

    // begin a debug label region and end it automatically with a RAII helper
    [[nodiscard]] auto scoped_debug_label(char const* label)
    {
        begin_debug_label(label);
        CC_RETURN_DEFER { this->end_debug_label(); };
    }

    //
    // specials

    // uploads texture data correctly to a destination texture, respecting rowwise alignment
    // transition cmd + copy_buf_to_tex cmd
    // expects an upload buffer with sufficient size (see Context::calculate_texture_upload_size)
    void upload_texture_data(cc::span<std::byte const> texture_data, buffer const& upload_buffer, texture const& dest_texture);

    // creates a suitable temporary upload buffer and calls upload_texture_data
    void auto_upload_texture_data(cc::span<std::byte const> texture_data, texture const& dest_texture);

    size_t upload_texture_subresource(cc::span<std::byte const> texture_data,
                                      uint32_t row_size_bytes,
                                      buffer const& upload_buffer,
                                      uint32_t buffer_offset_bytes,
                                      texture const& dest_texture,
                                      uint32_t dest_mip_index,
                                      uint32_t dest_array_index);

    // creates a suitable temporary upload buffer and copies data to the destination buffer
    void auto_upload_buffer_data(cc::span<std::byte const> data, buffer const& dest_buffer);

    // free a buffer once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(buffer const& buf) { free_deferred_after_submit(buf.handle); }

    // free a texture once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(texture const& tex) { free_deferred_after_submit(tex.handle); }

    // free a resource once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(resource const& res) { free_deferred_after_submit(res.handle); }

    // free raw PHI resources once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(phi::handle::resource res) { mDeferredFreeResources.push_back(res); }

    // free a buffer to the cache AFTER this frame was submitted/discarded
    void free_to_cache_after_submit(buffer const& buf) { free_to_cache_after_submit(buf.handle); }

    // free a texture to the cache AFTER this frame was submitted/discarded
    void free_to_cache_after_submit(texture const& tex) { free_to_cache_after_submit(tex.handle); }

    // free a resource to the cache AFTER this frame was submitted/discarded
    void free_to_cache_after_submit(resource const& res) { free_to_cache_after_submit(res.handle); }

    // free raw PHI resources to the cache AFTER this frame was submitted/discarded
    void free_to_cache_after_submit(phi::handle::resource res) { mCacheFreeResources.push_back(res); }

    // write multiple resource slice transitions - no state tracking
    void transition_slices(cc::span<phi::cmd::transition_image_slices::slice_transition_info const> slices);
    void transition_slices(phi::cmd::transition_image_slices const& tcmd);

    void begin_profile_scope(phi::cmd::begin_profile_scope const& bps);

    void end_profile_scope();

    //
    // raw command submission - advanced usage

    void raw_draw(phi::cmd::draw const& dcmd);

    void raw_draw_indirect(phi::cmd::draw_indirect const& dcmd);

    void raw_dipatch(phi::cmd::dispatch const& dcmd);

    void raw_dipatch_indirect(phi::cmd::dispatch_indirect const& dcmd);

    void raw_dispatch_rays(phi::cmd::dispatch_rays const& dcmd);

    void raw_clear_textures(phi::cmd::clear_textures const& ccmd);

    Context& context() { return *mCtx; }

    phi::handle::live_command_list get_list_handle() const { return mList; }

    size_t get_num_deferred_free_resources() const { return mDeferredFreeResources.size(); }

    size_t get_num_cache_free_resources() const { return mCacheFreeResources.size(); }

public:
    // redirect intuitive misuses

    // (graphics passes can only be created from framebuffers)
    [[deprecated("did you mean .make_framebuffer(..).make_pass(..)?")]] void make_pass(graphics_pipeline_state const&) = delete;

    // frame must not be discarded while framebuffers/passes are alive
    [[deprecated("pr::raii::Frame must stay alive while passes are used")]] ComputePass make_pass(compute_pipeline_state const&) && = delete;
    [[deprecated("pr::raii::Frame must stay alive while passes are used")]] ComputePass make_pass(phi::handle::pipeline_state) && = delete;
    [[deprecated("pr::raii::Frame must stay alive while passes are used")]] ComputePass make_pass(compute_pass_info const&) && = delete;
    [[deprecated("pr::raii::Frame must stay alive while framebuffers are used")]] Framebuffer make_framebuffer(phi::cmd::begin_render_pass const&) && = delete;
    template <class... RTs>
    [[deprecated("pr::raii::Frame must stay alive while framebuffers are used")]] Framebuffer make_framebuffer(RTs const&...) && = delete;
    [[deprecated("pr::raii::Frame must stay alive while framebuffers are used")]] framebuffer_builder build_framebuffer() && = delete;

    template <class T, auto_mode M>
    [[deprecated("auto_ types must not be explicitly freed, use .disown() for manual management")]] void free_deferred_after_submit(auto_destroyer<T, M> const&)
        = delete;
    template <class T, auto_mode M>
    [[deprecated("auto_ types must not be explicitly freed, use .disown() for manual management")]] void free_to_cache_deferred_after_submit(auto_destroyer<T, M> const&)
        = delete;

public:
    Frame(Frame const&) = delete;
    Frame& operator=(Frame const&) = delete;
    Frame(Frame&& rhs) noexcept;

    Frame& operator=(Frame&& rhs) noexcept;

    Frame() = default;
    ~Frame() { internalDestroy(); }

    // private
private:
    void addRenderTargetToFramebuffer(phi::cmd::begin_render_pass& bcmd, int& num_samples, texture const& rt);

    void flushPendingTransitions();

    void copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h, uint32_t mip_index, uint32_t first_array_index, uint32_t num_array_slices);
    void resolveTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h);

    phi::handle::pipeline_state acquireComputePSO(compute_pass_info const& cp);

    void internalDestroy();

    // framebuffer_builder-side API
private:
    friend struct framebuffer_builder;
    Framebuffer buildFramebuffer(phi::cmd::begin_render_pass const& bcmd, int num_samples, const phi::arg::framebuffer_config* blendstate_override, bool auto_transition);

    // Framebuffer-side API
private:
    friend class Framebuffer;
    void framebufferOnJoin(Framebuffer const&);
    // void framebufferOnSortByPSO(uint32_t num_drawcalls);

    phi::handle::pipeline_state framebufferAcquireGraphicsPSO(pr::graphics_pass_info const& gp, pr::framebuffer_info const& fb, int fb_inferred_num_samples);

    // Pass-side API
private:
    friend class GraphicsPass;
    friend class ComputePass;

    void passOnDispatch(phi::cmd::dispatch const& dcmd);

    phi::handle::shader_view passAcquireGraphicsShaderView(pr::argument& arg);
    phi::handle::shader_view passAcquireComputeShaderView(pr::argument& arg);

    // Context-side API
private:
    friend Context;
    explicit Frame(Context* ctx, phi::handle::live_command_list list, cc::allocator* alloc);

    void finalize();

    // members
private:
    phi::Backend* mBackend = nullptr;
    phi::handle::live_command_list mList = {};

    phi::cmd::transition_resources mPendingTransitionCommand;

    // cached objects (PSOs or SVs) to free once this frame is submitted or discarded
    cc::alloc_vector<freeable_cached_obj> mFreeables;

    // resources that should get deferred-freed once this frame is submitted or discarded
    cc::alloc_vector<phi::handle::resource> mDeferredFreeResources;

    // resources that should get freed-to-cache once this frame is submitted or discarde
    cc::alloc_vector<phi::handle::resource> mCacheFreeResources;

    Context* mCtx = nullptr;

    bool mFramebufferActive = false;
    phi::handle::swapchain mPresentAfterSubmitRequest = phi::handle::null_swapchain;
};
}
