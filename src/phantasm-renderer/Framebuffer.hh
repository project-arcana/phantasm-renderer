#pragma once

#include <clean-core/assert.hh>
#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/commands.hh>
#include <phantasm-hardware-interface/common/format_size.hh>

#include <phantasm-renderer/GraphicsPass.hh>
#include <phantasm-renderer/common/api.hh>
#include <phantasm-renderer/enums.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/pass_info.hh>

namespace pr::raii
{
class PR_API Framebuffer
{
public:
    // lvalue-qualified as Framebuffer has to stay alive

    /// start a graphics pass from persisted PSO
    [[nodiscard]] GraphicsPass make_pass(graphics_pipeline_state const& graphics_pipeline) &
    {
        CC_ASSERT(!mHasBlendstateOverrides && "blendstate settings are part of a PSO, not dynamic state. making blendstate settings in a Framebuffer has no effect on persistent PSOs");
        return {mParent, graphics_pipeline._handle};
    }

    /// starta graphics pass from a raw phi PSO
    [[nodiscard]] GraphicsPass make_pass(phi::handle::pipeline_state raw_pso) &
    {
        CC_ASSERT(!mHasBlendstateOverrides && "blendstate settings are part of a PSO, not dynamic state. making blendstate settings in a Framebuffer has no effect on persistent PSOs");
        return {mParent, raw_pso};
    }

    /// fetch a PSO from cache
    /// this hits a OS mutex and might have to build a PSO (expensive)
    [[nodiscard]] GraphicsPass make_pass(graphics_pass_info const& gp) &;

    /// sort previously recorded drawcalls by PSO - advanced feature
    /// requires #num_drawcalls contiguously recorded drawcalls
    void sort_drawcalls_by_pso(unsigned num_drawcalls);

public:
    // redirect intuitive misuses
    [[deprecated("pr::raii::Framebuffer must stay alive while passes are used")]] GraphicsPass make_pass(graphics_pipeline_state const&) && = delete;
    [[deprecated("pr::raii::Framebuffer must stay alive while passes are used")]] GraphicsPass make_pass(phi::handle::pipeline_state) && = delete;
    [[deprecated("pr::raii::Framebuffer must stay alive while passes are used")]] GraphicsPass make_pass(graphics_pass_info const&) && = delete;


public:
    Framebuffer(Framebuffer const&) = delete;
    Framebuffer(Framebuffer&& rhs) noexcept
      : mParent(rhs.mParent), mHashInfo(cc::move(rhs.mHashInfo)), mNumSamples(rhs.mNumSamples), mHasBlendstateOverrides(rhs.mHasBlendstateOverrides)
    {
        rhs.mParent = nullptr;
    }
    Framebuffer& operator=(Framebuffer const&) = delete;
    Framebuffer& operator=(Framebuffer&& rhs) noexcept
    {
        if (this != &rhs)
        {
            destroy();
            mParent = rhs.mParent;
            mHashInfo = cc::move(rhs.mHashInfo);
            mNumSamples = rhs.mNumSamples;
            mHasBlendstateOverrides = rhs.mHasBlendstateOverrides;
            rhs.mParent = nullptr;
        }

        return *this;
    }
    ~Framebuffer();

private:
    friend class Frame;
    Framebuffer(Frame* parent, framebuffer_info const& hashInfo, int num_samples, bool has_blendstate_overrides)
      : mParent(parent), mHashInfo(hashInfo), mNumSamples(num_samples), mHasBlendstateOverrides(has_blendstate_overrides)
    {
    }
    void destroy();

    Frame* mParent = nullptr;
    framebuffer_info mHashInfo;
    int mNumSamples = 0; ///< amount of multisamples of the rendertargets, inferred. unknown (-1) on some paths
    bool mHasBlendstateOverrides
        = false; ///< whether mHashInfo contains blendstate overrides - this triggers asserts if using persisted PSOs as they are unaffected by these settings
};

struct PR_API framebuffer_builder
{
public:
    /// add a rendertarget to the framebuffer that loads is contents (ie. is not cleared)
    [[nodiscard]] framebuffer_builder& loaded_target(render_target const& rt, uint32_t mip_index = 0u, uint32_t array_index = 0u)
    {
        if (phi::util::is_depth_format(rt.info.format))
        {
            _cmd.depth_target = phi::cmd::begin_render_pass::depth_stencil_info{{}, 1.f, 0, phi::rt_clear_type::load};
            _cmd.depth_target.rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1, mip_index, 1u, array_index);
        }
        else
        {
            _cmd.render_targets.push_back(phi::cmd::begin_render_pass::render_target_info{{}, {0.f, 0.f, 0.f, 1.f}, phi::rt_clear_type::load});
            _cmd.render_targets.back().rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1, mip_index, 1u, array_index);
        }
        adjust_config_for_render_target(rt);
        return *this;
    }

    /// add a rendertarget to the framebuffer that clears to a specified value
    [[nodiscard]] framebuffer_builder& cleared_target(
        render_target const& rt, float clear_r = 0.f, float clear_g = 0.f, float clear_b = 0.f, float clear_a = 1.f, uint32_t mip_index = 0u, uint32_t array_index = 0u)
    {
        CC_ASSERT(!phi::util::is_depth_format(rt.info.format) && "invoked clear_target color variant with a depth render target");

        _cmd.render_targets.push_back(phi::cmd::begin_render_pass::render_target_info{{}, {clear_r, clear_g, clear_b, clear_a}, phi::rt_clear_type::clear});
        _cmd.render_targets.back().rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1, mip_index, 1u, array_index);
        adjust_config_for_render_target(rt);
        return *this;
    }

    /// add a depth rendertarget to the framebuffer that clears to a specified value
    [[nodiscard]] framebuffer_builder& cleared_depth(render_target const& rt, float clear_depth = 1.f, uint8_t clear_stencil = 0, uint32_t mip_index = 0u, uint32_t array_index = 0u)
    {
        CC_ASSERT(phi::util::is_depth_format(rt.info.format) && "invoked clear_target depth variant with a non-depth render target");

        _cmd.depth_target = phi::cmd::begin_render_pass::depth_stencil_info{{}, clear_depth, clear_stencil, phi::rt_clear_type::clear};
        _cmd.depth_target.rv.init_as_tex2d(rt.res.handle, rt.info.format, rt.info.num_samples > 1, mip_index, 1u, array_index);
        adjust_config_for_render_target(rt);
        return *this;
    }

    /// add a rendertarget to the framebuffer that loads is contents (ie. is not cleared), including a blend state override
    /// NOTE: blend state only applies to cached PSOs created from the framebuffer
    [[nodiscard]] framebuffer_builder& loaded_target(render_target const& rt, pr::blend_state const& blend, uint32_t mip_index = 0u, uint32_t array_index = 0u)
    {
        CC_ASSERT(!phi::util::is_depth_format(rt.info.format) && "cannot specify blend state for depth targets");

        (void)loaded_target(rt, mip_index, array_index);
        _has_custom_blendstate = true;
        auto& state = _blendstate_overrides.render_targets.back();
        state.blend_enable = true;
        state.state = blend; // the words sing to me
        return *this;
    }

    /// add a rendertarget to the framebuffer that clears to a specified value, including a blend state override
    /// NOTE: blend state only applies to cached PSOs created from the framebuffer
    [[nodiscard]] framebuffer_builder& cleared_target(render_target const& rt,
                                                      pr::blend_state const& blend,
                                                      float clear_r = 0.f,
                                                      float clear_g = 0.f,
                                                      float clear_b = 0.f,
                                                      float clear_a = 1.f,
                                                      uint32_t mip_index = 0u,
                                                      uint32_t array_index = 0u)
    {
        CC_ASSERT(!phi::util::is_depth_format(rt.info.format) && "cannot specify blend state for depth targets");
        (void)cleared_target(rt, clear_r, clear_g, clear_b, clear_a, mip_index, array_index);
        _has_custom_blendstate = true;
        auto& state = _blendstate_overrides.render_targets.back();
        state.blend_enable = true;
        state.state = blend;
        return *this;
    }

    /// Set and enable the blend logic operation
    /// NOTE: only applies to cached PSOs created from the framebuffer
    [[nodiscard]] framebuffer_builder& logic_op(pr::blend_logic_op op)
    {
        _has_custom_blendstate = true;
        _blendstate_overrides.logic_op = op;
        _blendstate_overrides.logic_op_enable = true;
        return *this;
    }

    /// override the viewport (defaults to minimum of target sizes)
    [[nodiscard]] framebuffer_builder& set_viewport(int w, int h) { return this->set_viewport({w, h}); }

    /// override the viewport (defaults to minimum of target sizes)
    [[nodiscard]] framebuffer_builder& set_viewport(tg::isize2 size)
    {
        _cmd.viewport = size;
        _has_custom_viewport = true;
        return *this;
    }

    [[nodiscard]] framebuffer_builder& set_viewport_offset(tg::ivec2 offset)
    {
        _cmd.viewport_offset = offset;
        return *this;
    }

    /// disable automatical transitions of render and depth targets into fitting states
    [[nodiscard]] framebuffer_builder& no_transitions()
    {
        _wants_auto_transitions = false;
        return *this;
    }

    /// create the framebuffer
    [[nodiscard]] Framebuffer make();

    framebuffer_builder(framebuffer_builder const&) = delete;
    framebuffer_builder(framebuffer_builder&&) noexcept = default;

private:
    friend class Frame;
    framebuffer_builder(Frame* parent) : _parent(parent)
    {
        _cmd.viewport = {1 << 30, 1 << 30};
        _cmd.set_null_depth_stencil();
    }

    void adjust_config_for_render_target(render_target const& rt)
    {
        adjust_config_for_render_target(rt.info.num_samples, {rt.info.width, rt.info.height}, rt.info.format);
    }

    void adjust_config_for_render_target(uint32_t num_samples, tg::isize2 res, pr::format fmt)
    {
        CC_ASSERT((_num_samples == -1 || _num_samples == int(num_samples)) && "render targets in framebuffer have inconsistent sample amounts");
        _num_samples = int(num_samples);

        if (!_has_custom_viewport)
        {
            _cmd.viewport.width = cc::min(_cmd.viewport.width, res.width);
            _cmd.viewport.height = cc::min(_cmd.viewport.height, res.height);
        }

        // keep blenstate override RTs consistent in size
        if (!phi::util::is_depth_format(fmt))
            _blendstate_overrides.add_render_target(fmt);
    }

private:
    Frame* _parent;
    phi::cmd::begin_render_pass _cmd;
    phi::arg::framebuffer_config _blendstate_overrides;

    // if true, the viewport is no longer being inferred from additionally added render targets
    bool _has_custom_viewport = false;
    // if true, custom blendstate was specified
    bool _has_custom_blendstate = false;
    // if true, wants the frame to automatically insert transitions for all render targets
    bool _wants_auto_transitions = true;
    int _num_samples = -1; ///< amount of samples of the most recently added RT (-1 initially)
};
}
