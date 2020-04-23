#pragma once

#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/commands.hh>

#include <phantasm-renderer/GraphicsPass.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/pass_info.hh>

namespace pr::raii
{
class Framebuffer
{
public:
    Framebuffer(Frame* parent, framebuffer_info const& hashInfo) : mParent(parent), mHashInfo(hashInfo) {}

    Framebuffer(Framebuffer const&) = delete;
    Framebuffer(Framebuffer&& rhs) noexcept : mParent(rhs.mParent) { rhs.mParent = nullptr; }
    Framebuffer& operator=(Framebuffer const&) = delete;
    Framebuffer& operator=(Framebuffer&& rhs) noexcept
    {
        if (this != &rhs)
        {
            destroy();
            mParent = rhs.mParent;
            rhs.mParent = nullptr;
        }

        return *this;
    }

    ~Framebuffer();

public:
    // lvalue-qualified as Framebuffer has to stay alive

    /// start a graphics pass from persisted PSO
    [[nodiscard]] GraphicsPass make_pass(graphics_pipeline_state const& graphics_pipeline) & { return {mParent, graphics_pipeline._handle}; }

    /// starta graphics pass from a raw phi PSO
    [[nodiscard]] GraphicsPass make_pass(phi::handle::pipeline_state raw_pso) & { return {mParent, raw_pso}; }

    /// fetch a PSO from cache
    /// this hits a OS mutex and might have to build a PSO (expensive)
    [[nodiscard]] GraphicsPass make_pass(graphics_pass_info const& gp) &;

    // redirect intuitive misuses
    [[deprecated("pr::raii::Framebuffer must stay alive while passes are used")]] GraphicsPass make_pass(graphics_pipeline_state const&) && = delete;
    [[deprecated("pr::raii::Framebuffer must stay alive while passes are used")]] GraphicsPass make_pass(phi::handle::pipeline_state) && = delete;
    [[deprecated("pr::raii::Framebuffer must stay alive while passes are used")]] GraphicsPass make_pass(graphics_pass_info const&) && = delete;

private:
    void destroy();

    Frame* mParent;
    framebuffer_info mHashInfo;
};
struct framebuffer_builder
{
public:
    /// add a rendertarget to the framebuffer that loads is contents (ie. is not cleared)
    [[nodiscard]] framebuffer_builder& load_target(render_target const& rt)
    {
        if (phi::is_depth_format(rt.info.format))
        {
            _cmd.set_2d_depth_stencil(rt.res.handle, rt.info.format, phi::rt_clear_type::load, rt.info.num_samples > 1);
        }
        else
        {
            _cmd.add_2d_rt(rt.res.handle, rt.info.format, phi::rt_clear_type::load, rt.info.num_samples > 1);
        }
        adjust_viewport(rt);
        return *this;
    }

    /// add a rendertarget to the framebuffer that clears to a specified value
    [[nodiscard]] framebuffer_builder& clear_target(render_target const& rt, float clear_r = 0.f, float clear_g = 0.f, float clear_b = 0.f, float clear_a = 1.f)
    {
        CC_ASSERT(!phi::is_depth_format(rt.info.format) && "invoked clear_target color variant with a depth render target");
        _cmd.render_targets.push_back(phi::cmd::begin_render_pass::render_target_info{{}, {clear_r, clear_g, clear_b, clear_a}, phi::rt_clear_type::clear});
        _cmd.render_targets.back().rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1);
        adjust_viewport(rt);
        return *this;
    }

    /// add a depth rendertarget to the framebuffer that clears to a specified value
    [[nodiscard]] framebuffer_builder& clear_depth(render_target const& rt, float clear_depth = 1.f, uint8_t clear_stencil = 0)
    {
        CC_ASSERT(phi::is_depth_format(rt.info.format) && "invoked clear_target depth variant with a non-depth render target");
        _cmd.depth_target = phi::cmd::begin_render_pass::depth_stencil_info{{}, clear_depth, clear_stencil, phi::rt_clear_type::clear};
        _cmd.depth_target.rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1);
        adjust_viewport(rt);
        return *this;
    }

    /// override the viewport (defaults to minimum of target sizes)
    [[nodiscard]] framebuffer_builder& set_viewport(int w, int h)
    {
        _cmd.viewport = {w, h};
        _has_custom_viewport = true;
        return *this;
    }

    /// create the framebuffer
    [[nodiscard]] Framebuffer make();

    framebuffer_builder(framebuffer_builder const&) = delete;
    framebuffer_builder(framebuffer_builder&&) noexcept = default;

private:
    friend class Frame;
    framebuffer_builder(Frame* parent) : _parent(parent) { _cmd.viewport = {1 << 30, 1 << 30}; }

    void adjust_viewport(render_target const& rt)
    {
        if (_has_custom_viewport)
            return;
        _cmd.viewport.width = cc::min(_cmd.viewport.width, rt.info.width);
        _cmd.viewport.height = cc::min(_cmd.viewport.height, rt.info.height);
    }

private:
    Frame* _parent;
    phi::cmd::begin_render_pass _cmd;
    bool _has_custom_viewport = false;
};
}
