#pragma once

#include <clean-core/alloc_vector.hh>
#include <clean-core/defer.hh>
#include <clean-core/span.hh>

#include <typed-geometry/types/size.hh>

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/enums.hh>
#include <phantasm-renderer/fwd.hh>

#include <phantasm-renderer/resource_types.hh>

#include "ComputePass.hh"
#include "Framebuffer.hh"

namespace pr::raii
{
class Frame
{
public:
    //
    // framebuffer RAII API

    /// create a framebuffer by supplying render targets (all cleared, viewport minimum of sizes)
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

    /// create a framebuffer from a raw phi command
    [[nodiscard]] Framebuffer make_framebuffer(phi::cmd::begin_render_pass const& raw_command, bool auto_transition = true) &;

    /// create a framebuffer using a builder with more configuration options
    [[nodiscard]] framebuffer_builder build_framebuffer() & { return {this}; }

    //
    // pass RAII API (compute only, graphics passes are in Framebuffer)

    /// start a compute pass from persisted PSO
    [[nodiscard]] ComputePass make_pass(compute_pipeline_state const& compute_pipeline) & { return {this, compute_pipeline._handle}; }

    /// start a compute pass from a raw phi PSO
    [[nodiscard]] ComputePass make_pass(phi::handle::pipeline_state raw_pso) & { return {this, raw_pso}; }

    /// fetch a PSO from cache - this hits a OS mutex and might have to build a PSO (expensive)
    [[nodiscard]] ComputePass make_pass(compute_pass_info const& cp) &;

    //
    // transitions and present

    void transition(buffer const& res, state target, shader_flags dependency = {});
    void transition(texture const& res, state target, shader_flags dependency = {});
    void transition(render_target const& res, state target, shader_flags dependency = {});
    void transition(phi::handle::resource raw_resource, state target, shader_flags dependency = {});

    /// transition the backbuffer to present state and trigger a Context::present after this frame is submitted
    void present_after_submit(render_target const& backbuffer, swapchain sc);

    //
    // commands

    /// copy buffer to buffer
    void copy(buffer const& src, buffer const& dest, size_t src_offset = 0, size_t dest_offset = 0, size_t num_bytes = 0);
    /// copy buffer to texture
    void copy(buffer const& src, texture const& dest, size_t src_offset = 0, unsigned dest_mip_index = 0, unsigned dest_array_index = 0);

    /// copy texture to buffer
    void copy(render_target const& src, buffer const& dest, size_t dest_offset = 0);

    /// copies all array slices at the given MIP level from src to dest
    void copy(texture const& src, texture const& dest, unsigned mip_index = 0);
    /// copies MIP level 0, array slice 0 from src to dest
    void copy(texture const& src, render_target const& dest);
    /// copies src to MIP level 0, array slice 0 of dest
    void copy(render_target const& src, texture const& dest);
    /// copies contents of src to dest
    void copy(render_target const& src, render_target const& dest);

    /// copy textures specifying all details of the operation
    void copy_subsection(texture const& src,
                         texture const& dest,
                         unsigned src_mip_index,
                         unsigned src_array_index,
                         unsigned dest_mip_index,
                         unsigned dest_array_index,
                         unsigned num_array_slices,
                         tg::isize2 dest_size);

    /// resolve a multisampled render target to a texture or different RT
    void resolve(render_target const& src, texture const& dest);
    void resolve(render_target const& src, render_target const& dest);

    /// write a timestamp to a query in a given (timestamp) query range
    void write_timestamp(query_range const& query_range, unsigned index);

    /// resolve one or more queries in a range and write their contents to a buffer
    void resolve_queries(query_range const& src, buffer const& dest, unsigned first_query, unsigned num_queries, unsigned dest_offset_bytes = 0);

    /// begin a debug label region (visible in renderdoc, nsight, gpa, pix, etc.)
    void begin_debug_label(char const* label) { write_raw_cmd(phi::cmd::begin_debug_label{label}); }
    void end_debug_label() { write_raw_cmd(phi::cmd::end_debug_label{}); }

    /// begin a debug label region and end it automatically with a RAII helper
    [[nodiscard]] auto scoped_debug_label(char const* label)
    {
        begin_debug_label(label);
        CC_RETURN_DEFER { this->end_debug_label(); };
    }

    //
    // specials

    /// uploads texture data correctly to a destination texture, respecting rowwise alignment
    /// transition cmd + copy_buf_to_tex cmd
    /// expects upload buffer with sufficient size (see Context::calculate_texture_upload_size)
    void upload_texture_data(cc::span<std::byte const> texture_data, buffer const& upload_buffer, texture const& dest_texture);

    /// creates a suitable upload buffer and calls upload_texutre_data
    /// (deferred freeing it afterwards)
    void auto_upload_texture_data(cc::span<std::byte const> texture_data, texture const& dest_texture);

    size_t upload_texture_subresource(cc::span<std::byte const> texture_data,
                                      unsigned row_size_bytes,
                                      buffer const& upload_buffer,
                                      unsigned buffer_offset_bytes,
                                      texture const& dest_texture,
                                      unsigned dest_subres_index);

    /// free a buffer once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(buffer const& buf) { free_deferred_after_submit(buf.res.handle); }
    /// free a texture once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(texture const& tex) { free_deferred_after_submit(tex.res.handle); }
    /// free a render target once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(render_target const& rt) { free_deferred_after_submit(rt.res.handle); }
    /// free a resource once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(raw_resource const& res) { free_deferred_after_submit(res.handle); }
    /// free raw PHI resources once no longer in flight AFTER this frame was submitted/discarded
    void free_deferred_after_submit(phi::handle::resource res) { mDeferredFreeResources.push_back(res); }

    //
    // raw phi commands

    /// write a raw phi command
    template <class CmdT>
    void write_raw_cmd(CmdT const& cmd)
    {
        flushPendingTransitions();
        mWriter.add_command(cmd);
    }

    /// get a pointer to a buffer in order to write raw commands, must be written to immediately (before other writes)
    [[nodiscard]] std::byte* write_raw_bytes(size_t num_bytes)
    {
        flushPendingTransitions();
        return mWriter.write_raw_bytes(num_bytes);
    }

    /// write multiple resource slice transitions - no state tracking
    void transition_slices(cc::span<phi::cmd::transition_image_slices::slice_transition_info const> slices);

    Context& context() { return *mCtx; }

    bool is_empty() const { return mWriter.is_empty(); }

public:
    // redirect intuitive misuses
    /// (graphics passes can only be created from framebuffers)
    [[deprecated("did you mean .make_framebuffer(..).make_pass(..)?")]] void make_pass(graphics_pipeline_state const&) = delete;
    /// frame must not be discarded while framebuffers/passes are alive
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

public:
    Frame(Frame const&) = delete;
    Frame& operator=(Frame const&) = delete;
    Frame(Frame&& rhs) noexcept
      : mCtx(rhs.mCtx),
        mWriter(cc::move(rhs.mWriter)),
        mPendingTransitionCommand(rhs.mPendingTransitionCommand),
        mFreeables(cc::move(rhs.mFramebufferActive)),
        mDeferredFreeResources(cc::move(rhs.mDeferredFreeResources)),
        mFramebufferActive(rhs.mFramebufferActive),
        mPresentAfterSubmitRequest(rhs.mPresentAfterSubmitRequest)
    {
        rhs.mCtx = nullptr;
    }

    Frame& operator=(Frame&& rhs) noexcept;

    Frame() = default;
    ~Frame() { internalDestroy(); }

    // private
private:
    static void addRenderTargetToFramebuffer(phi::cmd::begin_render_pass& bcmd, int& num_samples, render_target const& rt);

    void flushPendingTransitions();

    void copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h, unsigned mip_index, unsigned first_array_index, unsigned num_array_slices);
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
    void framebufferOnSortByPSO(unsigned num_drawcalls);

    phi::handle::pipeline_state framebufferAcquireGraphicsPSO(pr::graphics_pass_info const& gp, pr::framebuffer_info const& fb, int fb_inferred_num_samples);

    // Pass-side API
private:
    friend class GraphicsPass;
    friend class ComputePass;

    void passOnDraw(phi::cmd::draw const& dcmd);
    void passOnDispatch(phi::cmd::dispatch const& dcmd);

    phi::handle::shader_view passAcquireGraphicsShaderView(pr::argument const& arg);
    phi::handle::shader_view passAcquireComputeShaderView(pr::argument const& arg);

    // Context-side API
private:
    friend Context;
    explicit Frame(Context* ctx, size_t size, cc::allocator* alloc)
      : mCtx(ctx), mWriter(size, alloc), mFreeables(alloc), mDeferredFreeResources(alloc)
    {
    }

    void finalize();
    std::byte* getMemory() const { return mWriter.buffer(); }
    size_t getSize() const { return mWriter.size(); }

    // members
private:
    Context* mCtx = nullptr;
    growing_writer mWriter;
    phi::cmd::transition_resources mPendingTransitionCommand;
    cc::alloc_vector<freeable_cached_obj> mFreeables;
    cc::alloc_vector<phi::handle::resource> mDeferredFreeResources;
    bool mFramebufferActive = false;
    phi::handle::swapchain mPresentAfterSubmitRequest = phi::handle::null_swapchain;
};
}
