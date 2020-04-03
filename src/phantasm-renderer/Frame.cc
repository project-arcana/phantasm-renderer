#include "Frame.hh"

#include <clean-core/utility.hh>

#include <phantasm-renderer/Context.hh>

#include "CompiledFrame.hh"

using namespace pr;

void Frame::transition(const buffer& res, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    transition(res._resource.data.handle, target, dependency);
}

void Frame::transition(const image& res, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    transition(res._resource.data.handle, target, dependency);
}

void Frame::transition(const render_target& res, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    transition(res._resource.data.handle, target, dependency);
}

void Frame::copy(const buffer& src, const buffer& dest, size_t src_offset, size_t dest_offset)
{
    transition(src, phi::resource_state::copy_src);
    transition(dest, phi::resource_state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_buffer ccmd;
    ccmd.init(src._resource.data.handle, dest._resource.data.handle, cc::min(src._info.size_bytes, dest._info.size_bytes), src_offset, dest_offset);
    mWriter.add_command(ccmd);
}

void Frame::copy(const buffer& src, const image& dest, size_t src_offset, unsigned dest_mip_index, unsigned dest_array_index)
{
    transition(src, phi::resource_state::copy_src);
    transition(dest, phi::resource_state::copy_dest);
    flushPendingTransitions();
    phi::cmd::copy_buffer_to_texture ccmd;
    ccmd.init(src._resource.data.handle, dest._resource.data.handle, dest._info.width / (1 + dest_mip_index),
              dest._info.height / (1 + dest_mip_index), src_offset, dest_mip_index, dest_array_index);
    mWriter.add_command(ccmd);
}

void Frame::copy(const image& src, const image& dest)
{
    copyTextureInternal(src._resource.data.handle, dest._resource.data.handle, dest._info.width, dest._info.height);
}

void Frame::copy(const image& src, const render_target& dest)
{
    copyTextureInternal(src._resource.data.handle, dest._resource.data.handle, dest._info.width, dest._info.height);
}

void Frame::copy(const render_target& src, const image& dest)
{
    copyTextureInternal(src._resource.data.handle, dest._resource.data.handle, dest._info.width, dest._info.height);
}

void Frame::copy(const render_target& src, const render_target& dest)
{
    copyTextureInternal(src._resource.data.handle, dest._resource.data.handle, dest._info.width, dest._info.height);
}

void Frame::resolve(const render_target& src, const image& dest)
{
    resolveTextureInternal(src._resource.data.handle, dest._resource.data.handle, dest._info.width, dest._info.height);
}

void Frame::resolve(const render_target& src, const render_target& dest)
{
    resolveTextureInternal(src._resource.data.handle, dest._resource.data.handle, dest._info.width, dest._info.height);
}

void Frame::addRenderTarget(phi::cmd::begin_render_pass& bcmd, const render_target& rt)
{
    auto const is_depth = phi::is_depth_format(rt._info.format);
    auto const target_state = is_depth ? phi::resource_state::depth_write : phi::resource_state::render_target;

    bcmd.viewport.width = cc::min(bcmd.viewport.width, rt._info.width);
    bcmd.viewport.height = cc::min(bcmd.viewport.height, rt._info.height);

    transition(rt._resource.data.handle, target_state);

    if (is_depth)
    {
        CC_ASSERT(!bcmd.depth_target.rv.resource.is_valid() && "passed multiple depth targets to Frame::render_to");
        bcmd.set_2d_depth_stencil(rt._resource.data.handle, rt._info.format, phi::rt_clear_type::clear, rt._info.num_samples > 1);
    }
    else
    {
        bcmd.add_2d_rt(rt._resource.data.handle, rt._info.format, phi::rt_clear_type::clear, rt._info.num_samples > 1);
    }
}

void Frame::transition(phi::handle::resource res, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    if (mPendingTransitionCommand.transitions.size() == phi::limits::max_resource_transitions)
        flushPendingTransitions();

    mPendingTransitionCommand.add(res, target, dependency);
}

void Frame::flushPendingTransitions()
{
    if (!mPendingTransitionCommand.transitions.empty())
    {
        mWriter.add_command(mPendingTransitionCommand);
        mPendingTransitionCommand.transitions.clear();
    }
}

void Frame::copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h)
{
    transition(src, phi::resource_state::copy_src);
    transition(dest, phi::resource_state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture ccmd;
    ccmd.init_symmetric(src, dest, unsigned(w), unsigned(h), 0);
    mWriter.add_command(ccmd);
}

void Frame::resolveTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h)
{
    transition(src, phi::resource_state::resolve_src);
    transition(dest, phi::resource_state::resolve_dest);
    flushPendingTransitions();

    phi::cmd::resolve_texture ccmd;
    ccmd.init_symmetric(src, dest, unsigned(w), unsigned(h));
    mWriter.add_command(ccmd);
}

Framebuffer Frame::buildFramebuffer(const phi::cmd::begin_render_pass& bcmd)
{
    for (auto const& rt : bcmd.render_targets)
    {
        transition(rt.rv.resource, phi::resource_state::render_target);
    }

    if (bcmd.depth_target.rv.resource.is_valid())
    {
        transition(bcmd.depth_target.rv.resource, phi::resource_state::depth_write);
    }

    flushPendingTransitions();
    mWriter.add_command(bcmd);
    return {this};
}

void Frame::framebufferOnJoin(const Framebuffer&)
{
    phi::cmd::end_render_pass ecmd;
    mWriter.add_command(ecmd);
}

void Frame::pipelineOnDraw(const phi::cmd::draw& dcmd) { mWriter.add_command(dcmd); }

void Frame::pipelineOnDispatch(const phi::cmd::dispatch& dcmd) { mWriter.add_command(dcmd); }
