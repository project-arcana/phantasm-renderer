#include "Frame.hh"

#include <clean-core/utility.hh>

#include <phantasm-renderer/Context.hh>

#include "CompiledFrame.hh"

using namespace pr;

raii::Framebuffer raii::Frame::make_framebuffer(const phi::cmd::begin_render_pass& raw_command) & { return buildFramebuffer(raw_command); }

raii::ComputePass raii::Frame::make_pass(const compute_pass_info& cp) & { return {this, acquireComputePSO(cp)}; }

void raii::Frame::transition(const buffer& res, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    transition(res.res.handle, target, dependency);
}

void raii::Frame::transition(const texture &res, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    transition(res.res.handle, target, dependency);
}

void raii::Frame::transition(const render_target& res, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    transition(res.res.handle, target, dependency);
}

void raii::Frame::transition(phi::handle::resource raw_resource, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    if (mPendingTransitionCommand.transitions.size() == phi::limits::max_resource_transitions)
        flushPendingTransitions();

    mPendingTransitionCommand.add(raw_resource, target, dependency);
}

void raii::Frame::copy(const buffer& src, const buffer& dest, size_t src_offset, size_t dest_offset)
{
    transition(src, phi::resource_state::copy_src);
    transition(dest, phi::resource_state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_buffer ccmd;
    ccmd.init(src.res.handle, dest.res.handle, cc::min(src.info.size_bytes, dest.info.size_bytes), src_offset, dest_offset);
    mWriter.add_command(ccmd);
}

void raii::Frame::copy(const buffer& src, const texture& dest, size_t src_offset, unsigned dest_mip_index, unsigned dest_array_index)
{
    transition(src, phi::resource_state::copy_src);
    transition(dest, phi::resource_state::copy_dest);
    flushPendingTransitions();
    phi::cmd::copy_buffer_to_texture ccmd;
    ccmd.init(src.res.handle, dest.res.handle, unsigned(dest.info.width) / (1 + dest_mip_index),
              unsigned(dest.info.height) / (1 + dest_mip_index), src_offset, dest_mip_index, dest_array_index);
    mWriter.add_command(ccmd);
}

void raii::Frame::copy(const texture& src, const texture& dest)
{
    copyTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::copy(const texture& src, const render_target& dest)
{
    copyTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::copy(const render_target& src, const texture& dest)
{
    copyTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::copy(const render_target& src, const render_target& dest)
{
    copyTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::resolve(const render_target& src, const texture &dest)
{
    resolveTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::resolve(const render_target& src, const render_target& dest)
{
    resolveTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::addRenderTarget(phi::cmd::begin_render_pass& bcmd, const render_target& rt)
{
    bcmd.viewport.width = cc::min(bcmd.viewport.width, rt.info.width);
    bcmd.viewport.height = cc::min(bcmd.viewport.height, rt.info.height);

    if (phi::is_depth_format(rt.info.format))
    {
        CC_ASSERT(!bcmd.depth_target.rv.resource.is_valid() && "passed multiple depth targets to raii::Frame::make_framebuffer");
        bcmd.set_2d_depth_stencil(rt.res.handle, rt.info.format, phi::rt_clear_type::clear, rt.info.num_samples > 1);
    }
    else
    {
        bcmd.add_2d_rt(rt.res.handle, rt.info.format, phi::rt_clear_type::clear, rt.info.num_samples > 1);
    }
}

void raii::Frame::flushPendingTransitions()
{
    if (!mPendingTransitionCommand.transitions.empty())
    {
        mWriter.add_command(mPendingTransitionCommand);
        mPendingTransitionCommand.transitions.clear();
    }
}

void raii::Frame::copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h)
{
    transition(src, phi::resource_state::copy_src);
    transition(dest, phi::resource_state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture ccmd;
    ccmd.init_symmetric(src, dest, unsigned(w), unsigned(h), 0);
    mWriter.add_command(ccmd);
}

void raii::Frame::resolveTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h)
{
    transition(src, phi::resource_state::resolve_src);
    transition(dest, phi::resource_state::resolve_dest);
    flushPendingTransitions();

    phi::cmd::resolve_texture ccmd;
    ccmd.init_symmetric(src, dest, unsigned(w), unsigned(h));
    mWriter.add_command(ccmd);
}

phi::handle::pipeline_state raii::Frame::acquireComputePSO(const compute_pass_info& cp)
{
    auto const cp_hash = cp.get_hash();
    auto const res = mCtx->acquire_compute_pso(cp_hash, cp);
    mFreeables.push_back({freeable_cached_obj::compute_pso, cp_hash});
    return res;
}

void raii::Frame::internalDestroy()
{
    if (mCtx != nullptr)
    {
        mCtx->free_all(mFreeables); // usually this is empty from a move during Context::compile(Frame&)
        mCtx = nullptr;
    }
}

raii::Framebuffer raii::Frame::buildFramebuffer(const phi::cmd::begin_render_pass& bcmd)
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


    // construct framebuffer info for future cache access inside of Framebuffer
    framebuffer_info fb_info;

    for (auto const& target : bcmd.render_targets)
        fb_info.target(target.rv.pixel_format);

    if (bcmd.depth_target.rv.resource.is_valid())
        fb_info.depth(bcmd.depth_target.rv.pixel_format);

    return {this, fb_info};
}

void raii::Frame::framebufferOnJoin(const raii::Framebuffer&)
{
    phi::cmd::end_render_pass ecmd;
    mWriter.add_command(ecmd);
}

phi::handle::pipeline_state raii::Frame::framebufferAcquireGraphicsPSO(const graphics_pass_info& gp, const framebuffer_info& fb)
{
    auto const gp_hash = gp.get_hash();
    auto const combined_hash = gp_hash.combine(fb.get_hash());
    auto const res = mCtx->acquire_graphics_pso(combined_hash, gp, fb);
    mFreeables.push_back({freeable_cached_obj::graphics_pso, combined_hash});
    return res;
}

void raii::Frame::passOnDraw(const phi::cmd::draw& dcmd) { mWriter.add_command(dcmd); }

void raii::Frame::passOnDispatch(const phi::cmd::dispatch& dcmd) { mWriter.add_command(dcmd); }

phi::handle::shader_view raii::Frame::passAcquireGraphicsShaderView(const argument& arg)
{
    murmur_hash hash;
    arg._info.get_murmur(hash);
    auto const res = mCtx->acquire_graphics_sv(hash, arg._info);
    mFreeables.push_back({freeable_cached_obj::graphics_sv, hash});
    return res;
}

phi::handle::shader_view raii::Frame::passAcquireComputeShaderView(const argument& arg)
{
    murmur_hash hash;
    arg._info.get_murmur(hash);
    auto const res = mCtx->acquire_compute_sv(hash, arg._info);
    mFreeables.push_back({freeable_cached_obj::compute_sv, hash});
    return res;
}

void raii::Frame::finalize() { flushPendingTransitions(); }
