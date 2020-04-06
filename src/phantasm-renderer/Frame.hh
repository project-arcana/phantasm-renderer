#pragma once

#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg.hh>

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/default_config.hh>
#include <phantasm-renderer/format.hh>

#include <phantasm-renderer/resource_types.hh>

#include "ComputePass.hh"
#include "Framebuffer.hh"

namespace pr::raii
{
class Frame
{
public:
    // pass RAII API

    /// create a framebuffer by supplying render targets (all cleared, viewport minimum of sizes)
    template <class... RTs>
    [[nodiscard]] Framebuffer make_framebuffer(RTs const&... targets) &
    {
        phi::cmd::begin_render_pass bcmd;
        // initialize command
        bcmd.set_null_depth_stencil();
        bcmd.viewport = tg::isize2(1 << 30, 1 << 30);

        // add targets
        (addRenderTarget(bcmd, targets), ...);
        return buildFramebuffer(bcmd);
    }

    /// create a framebuffer from a raw phi command
    [[nodiscard]] Framebuffer make_framebuffer(phi::cmd::begin_render_pass const& raw_command) &;

    /// create a framebuffer using a builder with more configuration options
    [[nodiscard]] framebuffer_builder build_framebuffer() & { return {this}; }

    // pipeline RAII API (compute only, graphics pipelines are in Framebuffer)

    /// start a compute pass from persisted PSO
    [[nodiscard]] ComputePass make_pass(compute_pipeline_state const& compute_pipeline) & { return {this, compute_pipeline.data._handle}; }
    /// starta compute pass from a raw phi PSO
    [[nodiscard]] ComputePass make_pass(phi::handle::pipeline_state raw_pso) & { return {this, raw_pso}; }

    /// fetch a PSO from cache - this hits a OS mutex and might have to build a PSO (expensive)
    [[nodiscard]] ComputePass make_pass(compute_pass_info const& cp) &;

    void transition(buffer const& res, phi::resource_state target, phi::shader_stage_flags_t dependency = {});
    void transition(texture const& res, phi::resource_state target, phi::shader_stage_flags_t dependency = {});
    void transition(render_target const& res, phi::resource_state target, phi::shader_stage_flags_t dependency = {});
    void transition(phi::handle::resource raw_resource, phi::resource_state target, phi::shader_stage_flags_t dependency = {});

    // commands

    void copy(buffer const& src, buffer const& dest, size_t src_offset = 0, size_t dest_offset = 0);
    void copy(buffer const& src, texture const& dest, size_t src_offset = 0, unsigned dest_mip_index = 0, unsigned dest_array_index = 0);

    void copy(texture const& src, texture const& dest);
    void copy(texture const& src, render_target const& dest);
    void copy(render_target const& src, texture const& dest);
    void copy(render_target const& src, render_target const& dest);

    void resolve(render_target const& src, texture const& dest);
    void resolve(render_target const& src, render_target const& dest);

    // raw command

    template <class CmdT>
    void write_raw_cmd(CmdT const& cmd)
    {
        mWriter.add_command(cmd);
    }

    // move-only type
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
    static void addRenderTarget(phi::cmd::begin_render_pass& bcmd, render_target const& rt);

    void flushPendingTransitions();

    void copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h);
    void resolveTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h);

    phi::handle::pipeline_state acquireComputePSO(compute_pass_info const& cp);

    void internalDestroy();

    // framebuffer_builder-side API
private:
    friend struct framebuffer_builder;
    Framebuffer buildFramebuffer(phi::cmd::begin_render_pass const& bcmd);

    // Framebuffer-side API
private:
    friend class Framebuffer;
    void framebufferOnJoin(Framebuffer const&);

    phi::handle::pipeline_state framebufferAcquireGraphicsPSO(pr::graphics_pass_info const& gp, pr::framebuffer_info const& fb);

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
    friend class Context;
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
};
}
