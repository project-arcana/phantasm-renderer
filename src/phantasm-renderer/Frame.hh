#pragma once

#include <clean-core/defer.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg.hh>

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/default_config.hh>
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
        return buildFramebuffer(bcmd, num_samples, nullptr);
    }

    /// create a framebuffer from a raw phi command
    [[nodiscard]] Framebuffer make_framebuffer(phi::cmd::begin_render_pass const& raw_command) &;

    /// create a framebuffer using a builder with more configuration options
    [[nodiscard]] framebuffer_builder build_framebuffer() & { return {this}; }

    //
    // pass RAII API (compute only, graphics passes are in Framebuffer)

    /// start a compute pass from persisted PSO
    [[nodiscard]] ComputePass make_pass(compute_pipeline_state const& compute_pipeline) & { return {this, compute_pipeline._handle}; }

    /// starta compute pass from a raw phi PSO
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
    void present_after_submit(render_target const& backbuffer)
    {
        CC_ASSERT(!mPresentAfterSubmitRequested && "only one present_after_submit per pr::raii::Frame allowed");
        transition(backbuffer, state::present);
        mPresentAfterSubmitRequested = true;
    }

    //
    // commands

    void copy(buffer const& src, buffer const& dest, size_t src_offset = 0, size_t dest_offset = 0);
    void copy(buffer const& src, texture const& dest, size_t src_offset = 0, unsigned dest_mip_index = 0, unsigned dest_array_index = 0);

    void copy(texture const& src, texture const& dest);
    void copy(texture const& src, render_target const& dest);
    void copy(render_target const& src, texture const& dest);
    void copy(render_target const& src, render_target const& dest);

    /// resolve a multisampled render target to a texture or different RT
    void resolve(render_target const& src, texture const& dest);
    void resolve(render_target const& src, render_target const& dest);

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

    /// uploads texture data correctly to a destination texture respecting rowwise alignment
    /// expects an appropriately sized upload buffer (see Context::calculate_texture_upload_size)
    void upload_texture_data(std::byte const* texture_data, buffer const& upload_buffer, texture const& dest_texture);

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

public:
    Frame(Frame const&) = delete;
    Frame& operator=(Frame const&) = delete;
    Frame(Frame&& rhs) noexcept : mCtx(rhs.mCtx), mWriter(cc::move(rhs.mWriter)), mPendingTransitionCommand(rhs.mPendingTransitionCommand)
    {
        rhs.mCtx = nullptr;
    }

    Frame& operator=(Frame&& rhs) noexcept
    {
        if (this != &rhs)
        {
            internalDestroy();
            mCtx = rhs.mCtx;
            mWriter = cc::move(rhs.mWriter);
            mPendingTransitionCommand = rhs.mPendingTransitionCommand;
            rhs.mCtx = nullptr;
        }

        return *this;
    }

    ~Frame() { internalDestroy(); }

    // private
private:
    static void addRenderTargetToFramebuffer(phi::cmd::begin_render_pass& bcmd, int& num_samples, render_target const& rt);

    void flushPendingTransitions();

    void copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h);
    void resolveTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h);

    phi::handle::pipeline_state acquireComputePSO(compute_pass_info const& cp);

    void internalDestroy();

    // framebuffer_builder-side API
private:
    friend struct framebuffer_builder;
    Framebuffer buildFramebuffer(phi::cmd::begin_render_pass const& bcmd, int num_samples, const phi::arg::framebuffer_config* blendstate_override);

    // Framebuffer-side API
private:
    friend class Framebuffer;
    void framebufferOnJoin(Framebuffer const&);

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
    Frame(Context* ctx, size_t size) : mCtx(ctx), mWriter(size) {}

    void finalize();
    std::byte* getMemory() const { return mWriter.buffer(); }
    size_t getSize() const { return mWriter.size(); }
    bool isEmpty() const { return mWriter.is_empty(); }

    // members
private:
    Context* mCtx;
    growing_writer mWriter;
    phi::cmd::transition_resources mPendingTransitionCommand;
    cc::vector<freeable_cached_obj> mFreeables;
    bool mFramebufferActive = false;
    bool mPresentAfterSubmitRequested = false;
};
}
