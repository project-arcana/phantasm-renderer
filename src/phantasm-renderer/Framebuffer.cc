#include "Framebuffer.hh"

#include "Frame.hh"

pr::raii::Framebuffer::~Framebuffer() { destroy(); }

pr::raii::GraphicsPass pr::raii::Framebuffer::make_pass(const pr::graphics_pass_info& gp) &
{
    return {mParent, mParent->framebufferAcquireGraphicsPSO(gp, mHashInfo, mNumSamples)};
}

void pr::raii::Framebuffer::destroy()
{
    if (mParent)
        mParent->framebufferOnJoin(*this);
}


pr::raii::framebuffer_builder& pr::raii::framebuffer_builder::loaded_target(texture const& rt, uint32_t mip_index, uint32_t array_index)
{
    auto const& texDesc = _parent->mCtx->get_backend().getResourceTextureDescription(rt.handle);
    if (phi::util::is_depth_format(texDesc.fmt))
    {
        _cmd.depth_target = phi::cmd::begin_render_pass::depth_stencil_info{{}, 1.f, 0, phi::rt_clear_type::load};

        if (array_index > 0)
        {
            _cmd.depth_target.rv.init_as_tex2d_array(rt.handle, texDesc.fmt, texDesc.num_samples > 1, array_index, 1u, mip_index);
        }
        else
        {
            _cmd.depth_target.rv.init_as_tex2d(rt.handle, texDesc.fmt, texDesc.num_samples > 1, mip_index);
        }
    }
    else
    {
        _cmd.render_targets.push_back(phi::cmd::begin_render_pass::render_target_info{{}, {0.f, 0.f, 0.f, 1.f}, phi::rt_clear_type::load});

        if (array_index > 0)
        {
            _cmd.render_targets.back().rv.init_as_tex2d_array(rt.handle, texDesc.fmt, texDesc.num_samples > 1, array_index, 1u, mip_index);
        }
        else
        {
            _cmd.render_targets.back().rv.init_as_tex2d(rt.handle, texDesc.fmt, texDesc.num_samples > 1, mip_index);
        }
    }
    adjust_config_for_render_target(rt);
    return *this;
}

pr::raii::framebuffer_builder& pr::raii::framebuffer_builder::cleared_target(
    texture const& rt, float clear_r, float clear_g, float clear_b, float clear_a, uint32_t mip_index, uint32_t array_index)
{
    auto const& texDesc = _parent->mCtx->get_backend().getResourceTextureDescription(rt.handle);
    CC_ASSERT(!phi::util::is_depth_format(texDesc.fmt) && "invoked clear_target color variant with a depth render target");

    _cmd.render_targets.push_back(phi::cmd::begin_render_pass::render_target_info{{}, {clear_r, clear_g, clear_b, clear_a}, phi::rt_clear_type::clear});
    if (array_index > 0)
    {
        _cmd.render_targets.back().rv.init_as_tex2d_array(rt.handle, texDesc.fmt, texDesc.num_samples > 1, array_index, 1u, mip_index);
    }
    else
    {
        _cmd.render_targets.back().rv.init_as_tex2d(rt.handle, texDesc.fmt, texDesc.num_samples > 1, mip_index);
    }

    adjust_config_for_render_target(rt);
    return *this;
}

pr::raii::framebuffer_builder& pr::raii::framebuffer_builder::cleared_depth(texture const& rt, float clear_depth, uint8_t clear_stencil, uint32_t mip_index, uint32_t array_index)
{
    auto const& texDesc = _parent->mCtx->get_backend().getResourceTextureDescription(rt.handle);
    CC_ASSERT(phi::util::is_depth_format(texDesc.fmt) && "invoked clear_target depth variant with a non-depth render target");

    _cmd.depth_target = phi::cmd::begin_render_pass::depth_stencil_info{{}, clear_depth, clear_stencil, phi::rt_clear_type::clear};

    if (array_index > 0)
    {
        _cmd.depth_target.rv.init_as_tex2d_array(rt.handle, texDesc.fmt, texDesc.num_samples > 1, array_index, 1u, mip_index);
    }
    else
    {
        _cmd.depth_target.rv.init_as_tex2d(rt.handle, texDesc.fmt, texDesc.num_samples > 1, mip_index);
    }
    adjust_config_for_render_target(rt);
    return *this;
}

pr::raii::framebuffer_builder& pr::raii::framebuffer_builder::loaded_target(texture const& rt, pr::blend_state const& blend, uint32_t mip_index, uint32_t array_index)
{
    auto const& texDesc = _parent->mCtx->get_backend().getResourceTextureDescription(rt.handle);
    CC_ASSERT(!phi::util::is_depth_format(texDesc.fmt) && "cannot specify blend state for depth targets");

    (void)loaded_target(rt, mip_index, array_index);
    _has_custom_blendstate = true;
    auto& state = _blendstate_overrides.render_targets.back();
    state.blend_enable = true;
    state.state = blend; // the words sing to me
    return *this;
}

pr::raii::framebuffer_builder& pr::raii::framebuffer_builder::cleared_target(
    texture const& rt, pr::blend_state const& blend, float clear_r, float clear_g, float clear_b, float clear_a, uint32_t mip_index, uint32_t array_index)
{
    auto const& texDesc = _parent->mCtx->get_backend().getResourceTextureDescription(rt.handle);
    CC_ASSERT(!phi::util::is_depth_format(texDesc.fmt) && "cannot specify blend state for depth targets");
    (void)cleared_target(rt, clear_r, clear_g, clear_b, clear_a, mip_index, array_index);
    _has_custom_blendstate = true;
    auto& state = _blendstate_overrides.render_targets.back();
    state.blend_enable = true;
    state.state = blend;
    return *this;
}

pr::raii::Framebuffer pr::raii::framebuffer_builder::make()
{
    return _parent->buildFramebuffer(_cmd, _num_samples, _has_custom_blendstate ? &_blendstate_overrides : nullptr, _wants_auto_transitions);
}

void pr::raii::framebuffer_builder::adjust_config_for_render_target(texture const& rt)
{
    auto const& texDesc = _parent->mCtx->get_backend().getResourceTextureDescription(rt.handle);
    adjust_config_for_render_target(texDesc.num_samples, {texDesc.width, texDesc.height}, texDesc.fmt);
}

void pr::raii::framebuffer_builder::adjust_config_for_render_target(uint32_t num_samples, tg::isize2 res, pr::format fmt)
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
