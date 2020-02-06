#include "Frame.hh"

#include <clean-core/utility.hh>

#include <phantasm-renderer/Context.hh>

#include "CompiledFrame.hh"

using namespace pr;

void Frame::copy(const buffer& src, const buffer& dest, size_t src_offset, size_t dest_offset)
{
    phi::cmd::transition_resources tcmd;
    tcmd.add(src._resource._handle, phi::resource_state::copy_src);
    tcmd.add(dest._resource._handle, phi::resource_state::copy_dest);
    mWriter.add_command(tcmd);

    phi::cmd::copy_buffer ccmd;
    ccmd.init(src._resource._handle, dest._resource._handle, cc::min(src._info.size_bytes, dest._info.size_bytes), src_offset, dest_offset);
    mWriter.add_command(ccmd);
}

void Frame::copy(const buffer& src, const image& dest, size_t src_offset, unsigned dest_mip_index, unsigned dest_array_index)
{
    phi::cmd::transition_resources tcmd;
    tcmd.add(src._resource._handle, phi::resource_state::copy_src);
    tcmd.add(dest._resource._handle, phi::resource_state::copy_dest);
    mWriter.add_command(tcmd);

    phi::cmd::copy_buffer_to_texture ccmd;
    ccmd.init(src._resource._handle, dest._resource._handle, dest._info.width / (1 + dest_mip_index), dest._info.height / (1 + dest_mip_index),
              src_offset, dest_mip_index, dest_array_index);
    mWriter.add_command(ccmd);
}

void Frame::copy(const image& src, const image& dest)
{
    phi::cmd::transition_resources tcmd;
    tcmd.add(src._resource._handle, phi::resource_state::copy_src);
    tcmd.add(dest._resource._handle, phi::resource_state::copy_dest);
    mWriter.add_command(tcmd);

    phi::cmd::copy_texture ccmd;
    ccmd.init_symmetric(src._resource._handle, dest._resource._handle, dest._info.width, dest._info.height, 0);
    mWriter.add_command(ccmd);
}

void Frame::resolve(const render_target& src, const image& dest)
{
    phi::cmd::transition_resources tcmd;
    tcmd.add(src._resource._handle, phi::resource_state::resolve_src);
    tcmd.add(dest._resource._handle, phi::resource_state::resolve_dest);
    mWriter.add_command(tcmd);

    phi::cmd::resolve_texture ccmd;
    ccmd.init_symmetric(src._resource._handle, dest._resource._handle, dest._info.width, dest._info.height);
    mWriter.add_command(ccmd);
}

void Frame::addRenderTarget(phi::cmd::begin_render_pass& bcmd, const render_target& rt)
{
    auto const is_depth = phi::is_depth_format(rt._info.format);
    auto const target_state = is_depth ? phi::resource_state::depth_write : phi::resource_state::render_target;

    addPendingTransition(rt._resource._handle, target_state);

    if (is_depth)
    {
        CC_ASSERT(!bcmd.depth_target.rv.resource.is_valid() && "passed multiple depth targets to Frame::render_to");
        bcmd.set_2d_depth_stencil(rt._resource._handle, rt._info.format, phi::rt_clear_type::clear, rt._info.num_samples > 1);
    }
    else
    {
        bcmd.add_2d_rt(rt._resource._handle, rt._info.format, phi::rt_clear_type::clear, rt._info.num_samples > 1);
    }
}

void Frame::addPendingTransition(phi::handle::resource res, phi::resource_state target, phi::shader_stage_flags_t dependency)
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
