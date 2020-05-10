#include "Frame.hh"

#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/detail/byte_util.hh>
#include <phantasm-hardware-interface/detail/format_size.hh>

#include <phantasm-renderer/Context.hh>

#include "CompiledFrame.hh"


namespace
{
void rowwise_copy(std::byte const* src, std::byte* dest, unsigned dest_row_stride_bytes, unsigned row_size_bytes, unsigned height_pixels)
{
    for (auto y = 0u; y < height_pixels; ++y)
    {
        std::memcpy(dest + y * dest_row_stride_bytes, src + y * row_size_bytes, row_size_bytes);
    }
}
}

using namespace pr;

raii::Framebuffer raii::Frame::make_framebuffer(const phi::cmd::begin_render_pass& raw_command) &
{
    return buildFramebuffer(raw_command, -1, nullptr);
}

raii::ComputePass raii::Frame::make_pass(const compute_pass_info& cp) & { return {this, acquireComputePSO(cp)}; }

void raii::Frame::transition(const buffer& res, phi::resource_state target, phi::shader_stage_flags_t dependency)
{
    if (res.info.heap != resource_heap::gpu) // mapped buffers are never transitioned
        return;

    transition(res.res.handle, target, dependency);
}

void raii::Frame::transition(const texture& res, phi::resource_state target, phi::shader_stage_flags_t dependency)
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
    ccmd.init(src.res.handle, dest.res.handle, unsigned(dest.info.width) / (1 + dest_mip_index), unsigned(dest.info.height) / (1 + dest_mip_index),
              src_offset, dest_mip_index, dest_array_index);
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

void raii::Frame::resolve(const render_target& src, const texture& dest)
{
    resolveTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::resolve(const render_target& src, const render_target& dest)
{
    resolveTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::upload_texture_data(std::byte const* texture_data, const buffer& upload_buffer, const texture& dest_texture)
{
    transition(dest_texture, phi::resource_state::copy_dest);
    flushPendingTransitions();
    CC_ASSERT(dest_texture.info.depth_or_array_size == 1 && "array upload unimplemented");
    CC_ASSERT(upload_buffer.info.heap == resource_heap::upload && "buffer is not an upload buffer");

    auto const bytes_per_pixel = phi::detail::format_size_bytes(dest_texture.info.fmt);
    auto const use_d3d12_per_row_alingment = mCtx->get_backend().getBackendType() == phi::backend_type::d3d12;
    auto* const upload_buffer_map = mCtx->map_buffer(upload_buffer);

    phi::cmd::copy_buffer_to_texture command;
    command.source = upload_buffer.res.handle;
    command.destination = dest_texture.res.handle;
    command.dest_width = unsigned(dest_texture.info.width);
    command.dest_height = unsigned(dest_texture.info.height);
    command.dest_mip_index = 0;

    auto accumulated_offset_bytes = 0u;

    // for (auto a = 0u; a < img_size.array_size; ++a)
    {
        command.dest_array_index = 0u; // a;
        command.source_offset = accumulated_offset_bytes;

        mWriter.add_command(command);

        auto const mip_row_size_bytes = bytes_per_pixel * command.dest_width;
        auto mip_row_stride_bytes = mip_row_size_bytes;

        // MIP maps are 256-byte aligned per row in d3d12
        if (use_d3d12_per_row_alingment)
            mip_row_stride_bytes = phi::mem::align_up(mip_row_stride_bytes, 256);

        auto const mip_offset_bytes = mip_row_stride_bytes * command.dest_height;
        accumulated_offset_bytes += mip_offset_bytes;

        rowwise_copy(texture_data, upload_buffer_map + command.source_offset, mip_row_stride_bytes, mip_row_size_bytes, command.dest_height);
    }

    mCtx->unmap_buffer(upload_buffer);
}

void raii::Frame::transition_slices(cc::span<const phi::cmd::transition_image_slices::slice_transition_info> slices)
{
    flushPendingTransitions();

    phi::cmd::transition_image_slices tcmd;
    for (auto const& ti : slices)
    {
        if (tcmd.transitions.size() == phi::limits::max_resource_transitions)
        {
            mWriter.add_command(tcmd);
            tcmd.transitions.clear();
        }

        tcmd.transitions.push_back(ti);
    }

    if (!tcmd.transitions.empty())
        mWriter.add_command(tcmd);
}

void raii::Frame::addRenderTargetToFramebuffer(phi::cmd::begin_render_pass& bcmd, int& num_samples, const render_target& rt)
{
    bcmd.viewport.width = cc::min(bcmd.viewport.width, rt.info.width);
    bcmd.viewport.height = cc::min(bcmd.viewport.height, rt.info.height);

    if (num_samples != -1)
    {
        CC_ASSERT(num_samples == int(rt.info.num_samples) && "make_framebuffer: inconsistent amount of samples in render targets");
    }

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
        CC_ASSERT(!mFramebufferActive && "No transitions allowed during active raii::Framebuffers");
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

raii::Framebuffer raii::Frame::buildFramebuffer(const phi::cmd::begin_render_pass& bcmd, int num_samples, const phi::arg::framebuffer_config* blendstate_override)
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
    mFramebufferActive = true;
    mWriter.add_command(bcmd);

    bool const has_blendstate_overrides = blendstate_override != nullptr;
    // construct framebuffer info for future cache access inside of Framebuffer
    framebuffer_info fb_info;

    for (auto const& target : bcmd.render_targets)
        fb_info.target(target.rv.pixel_format);

    if (bcmd.depth_target.rv.resource.is_valid())
        fb_info.depth(bcmd.depth_target.rv.pixel_format);

    if (has_blendstate_overrides)
    {
        // framebuffer_info was provided with additional blendstate information, override into fb_info
        fb_info._storage.get().logic_op_enable = blendstate_override->logic_op_enable;
        fb_info._storage.get().logic_op = blendstate_override->logic_op;

        CC_ASSERT(fb_info._storage.get().render_targets.size() == blendstate_override->render_targets.size() && "inconsistent blendstate override");

        for (uint8_t i = 0u; i < blendstate_override->render_targets.size(); ++i)
        {
            if (blendstate_override->render_targets[i].blend_enable)
            {
                auto& target_rt = fb_info._storage.get().render_targets[i];

                target_rt.blend_enable = true;
                target_rt.state = blendstate_override->render_targets[i].state;
            }
        }
    }

    return {this, fb_info, num_samples, has_blendstate_overrides};
}

void raii::Frame::framebufferOnJoin(const raii::Framebuffer&)
{
    phi::cmd::end_render_pass ecmd;
    mWriter.add_command(ecmd);
    mFramebufferActive = false;
}

phi::handle::pipeline_state raii::Frame::framebufferAcquireGraphicsPSO(const graphics_pass_info& gp, const framebuffer_info& fb, int fb_inferred_num_samples)
{
    CC_ASSERT(fb_inferred_num_samples == -1
              || fb_inferred_num_samples == gp._storage.get().graphics_config.samples && "graphics_pass_info has incorrect amount of samples configured");
    auto const gp_hash = gp.get_hash();
    auto const combined_hash = gp_hash.combine(fb.get_hash());
    auto const res = mCtx->acquire_graphics_pso(combined_hash, gp, fb);
    mFreeables.push_back({freeable_cached_obj::graphics_pso, combined_hash});
    return res;
}

void raii::Frame::passOnDraw(const phi::cmd::draw& dcmd) { mWriter.add_command(dcmd); }

void raii::Frame::passOnDispatch(const phi::cmd::dispatch& dcmd)
{
    flushPendingTransitions();
    mWriter.add_command(dcmd);
}

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
