#include "Frame.hh"

#include <algorithm> // std::sort

#include <clean-core/hash_combine.hh>
#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/detail/byte_util.hh>
#include <phantasm-hardware-interface/detail/format_size.hh>

#include <phantasm-renderer/Context.hh>
#include <phantasm-renderer/common/log.hh>

#include "CompiledFrame.hh"


namespace
{
void rowwise_copy(cc::span<std::byte const> src, std::byte* dest, unsigned dest_row_stride_bytes, unsigned row_size_bytes, unsigned height_pixels)
{
    for (auto y = 0u; y < height_pixels; ++y)
    {
        std::memcpy(dest + y * dest_row_stride_bytes, src.data() + y * row_size_bytes, row_size_bytes);
    }
}
}

using namespace pr;

raii::Framebuffer raii::Frame::make_framebuffer(const phi::cmd::begin_render_pass& raw_command, bool auto_transition) &
{
    return buildFramebuffer(raw_command, -1, nullptr, auto_transition);
}

raii::Frame& raii::Frame::operator=(raii::Frame&& rhs) noexcept
{
    if (this != &rhs)
    {
        internalDestroy();
        mCtx = rhs.mCtx;
        mWriter = cc::move(rhs.mWriter);
        mPendingTransitionCommand = rhs.mPendingTransitionCommand;
        mFreeables = cc::move(rhs.mFreeables);
        mFramebufferActive = rhs.mFramebufferActive;
        mPresentAfterSubmitRequest = rhs.mPresentAfterSubmitRequest;
        rhs.mCtx = nullptr;
    }

    return *this;
}

raii::ComputePass raii::Frame::make_pass(const compute_pass_info& cp) & { return {this, acquireComputePSO(cp)}; }

void raii::Frame::transition(const buffer& res, pr::state target, shader_flags dependency)
{
    if (res.info.heap != resource_heap::gpu) // mapped buffers are never transitioned
        return;

    transition(res.res.handle, target, dependency);
}

void raii::Frame::transition(const texture& res, pr::state target, shader_flags dependency) { transition(res.res.handle, target, dependency); }

void raii::Frame::transition(const render_target& res, pr::state target, shader_flags dependency) { transition(res.res.handle, target, dependency); }

void raii::Frame::transition(phi::handle::resource raw_resource, pr::state target, shader_flags dependency)
{
    if (mPendingTransitionCommand.transitions.size() == phi::limits::max_resource_transitions)
        flushPendingTransitions();

    mPendingTransitionCommand.add(raw_resource, target, dependency);
}

void raii::Frame::present_after_submit(const render_target& backbuffer, swapchain sc)
{
    CC_ASSERT(!mPresentAfterSubmitRequest.is_valid() && "only one present_after_submit per pr::raii::Frame allowed");
    transition(backbuffer, state::present);
    mPresentAfterSubmitRequest = sc.handle;
}

void raii::Frame::copy(const buffer& src, const buffer& dest, size_t src_offset, size_t dest_offset, size_t num_bytes)
{
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_buffer ccmd;
    ccmd.init(src.res.handle, dest.res.handle, num_bytes > 0 ? num_bytes : cc::min(src.info.size_bytes, dest.info.size_bytes), src_offset, dest_offset);
    mWriter.add_command(ccmd);
}

void raii::Frame::copy(const buffer& src, const texture& dest, size_t src_offset, unsigned dest_mip_index, unsigned dest_array_index)
{
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();
    phi::cmd::copy_buffer_to_texture ccmd;
    ccmd.init(src.res.handle, dest.res.handle, unsigned(dest.info.width) / (1 + dest_mip_index), unsigned(dest.info.height) / (1 + dest_mip_index),
              src_offset, dest_mip_index, dest_array_index);
    mWriter.add_command(ccmd);
}

void raii::Frame::copy(const render_target& src, const buffer& dest, size_t dest_offset)
{
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture_to_buffer ccmd;
    ccmd.init(src.res.handle, dest.res.handle, src.info.width, src.info.height, dest_offset);
    mWriter.add_command(ccmd);
}

void raii::Frame::copy(const texture& src, const texture& dest, unsigned mip_index)
{
    CC_ASSERT(src.info.width == dest.info.width && "copy size mismatch");
    CC_ASSERT(src.info.height == dest.info.height && "copy size mismatch");
    CC_ASSERT(mip_index < dest.info.num_mips && mip_index < src.info.num_mips && "mip index out of bounds");
    copyTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height, mip_index, 0, dest.info.depth_or_array_size);
}

void raii::Frame::copy(const texture& src, const render_target& dest)
{
    CC_ASSERT(src.info.width == dest.info.width && "copy size mismatch");
    CC_ASSERT(src.info.height == dest.info.height && "copy size mismatch");
    copyTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height, 0, 0, 1);
}

void raii::Frame::copy(const render_target& src, const texture& dest)
{
    CC_ASSERT(src.info.width == dest.info.width && "copy size mismatch");
    CC_ASSERT(src.info.height == dest.info.height && "copy size mismatch");
    copyTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height, 0, 0, 1);
}

void raii::Frame::copy(const render_target& src, const render_target& dest)
{
    CC_ASSERT(src.info.width == dest.info.width && "copy size mismatch");
    CC_ASSERT(src.info.height == dest.info.height && "copy size mismatch");
    copyTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height, 0, 0, 1);
}

void raii::Frame::copy_subsection(const texture& src,
                                  const texture& dest,
                                  unsigned src_mip_index,
                                  unsigned src_array_index,
                                  unsigned dest_mip_index,
                                  unsigned dest_array_index,
                                  unsigned num_array_slices,
                                  tg::isize2 dest_size)
{
    // TODO: OOB asserts
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture ccmd;
    ccmd.source = src.res.handle;
    ccmd.destination = src.res.handle;
    ccmd.src_mip_index = src_mip_index;
    ccmd.src_array_index = src_array_index;
    ccmd.dest_mip_index = dest_mip_index;
    ccmd.dest_array_index = dest_array_index;
    ccmd.width = unsigned(dest_size.width);
    ccmd.height = unsigned(dest_size.height);
    ccmd.num_array_slices = num_array_slices;
    mWriter.add_command(ccmd);
}

void raii::Frame::resolve(const render_target& src, const texture& dest)
{
    resolveTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::resolve(const render_target& src, const render_target& dest)
{
    resolveTextureInternal(src.res.handle, dest.res.handle, dest.info.width, dest.info.height);
}

void raii::Frame::write_timestamp(const query_range& query_range, unsigned index)
{
    CC_ASSERT(query_range.type == query_type::timestamp && "Expected timestamp query range to write a timestamp to");
    CC_ASSERT(index < query_range.num && "OOB query write");

    phi::cmd::write_timestamp wcmd;
    wcmd.index = index;
    wcmd.query_range = query_range.handle;
    mWriter.add_command(wcmd);
}

void raii::Frame::resolve_queries(const query_range& src, const buffer& dest, unsigned first_query, unsigned num_queries, unsigned dest_offset_bytes)
{
    CC_ASSERT(first_query + num_queries <= src.num && "OOB query resolve read");
    CC_ASSERT(dest_offset_bytes + num_queries * sizeof(uint64_t) <= dest.info.size_bytes && "OOB query resolve write");
    flushPendingTransitions();

    phi::cmd::resolve_queries rcmd;
    rcmd.init(dest.res.handle, src.handle, first_query, num_queries, dest_offset_bytes);
    mWriter.add_command(rcmd);
}

void raii::Frame::upload_texture_data(cc::span<const std::byte> texture_data, const buffer& upload_buffer, const texture& dest_texture)
{
    transition(dest_texture, pr::state::copy_dest);
    flushPendingTransitions();
    CC_ASSERT(dest_texture.info.depth_or_array_size == 1 && "array upload unimplemented");
    CC_ASSERT(upload_buffer.info.heap == resource_heap::upload && "buffer is not an upload buffer");

    auto const bytes_per_pixel = phi::util::get_format_size_bytes(dest_texture.info.fmt);
    auto const use_d3d12_per_row_alingment = mCtx->get_backend_type() == pr::backend::d3d12;
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
            mip_row_stride_bytes = phi::util::align_up(mip_row_stride_bytes, 256);

        auto const mip_offset_bytes = mip_row_stride_bytes * command.dest_height;
        accumulated_offset_bytes += mip_offset_bytes;

        CC_ASSERT(texture_data.size() >= mip_row_size_bytes * command.dest_height && "texture source data too small");
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

void raii::Frame::copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h, unsigned mip_index, unsigned first_array_index, unsigned num_array_slices)
{
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture ccmd;
    ccmd.init_symmetric(src, dest, unsigned(w), unsigned(h), mip_index, first_array_index, num_array_slices);
    mWriter.add_command(ccmd);
}

void raii::Frame::resolveTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h)
{
    transition(src, pr::state::resolve_src);
    transition(dest, pr::state::resolve_dest);
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

raii::Framebuffer raii::Frame::buildFramebuffer(const phi::cmd::begin_render_pass& bcmd, int num_samples, const phi::arg::framebuffer_config* blendstate_override, bool auto_transition)
{
    if (auto_transition)
    {
        for (auto const& rt : bcmd.render_targets)
        {
            transition(rt.rv.resource, pr::state::render_target);
        }

        if (bcmd.depth_target.rv.resource.is_valid())
        {
            transition(bcmd.depth_target.rv.resource, pr::state::depth_write);
        }

        flushPendingTransitions();
    }

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

void raii::Frame::framebufferOnSortByPSO(unsigned num_drawcalls)
{
    // sort previously recorded draw commands by PSO, directly in the command buffer
    // requires strictly NOTHING but drawcalls recorded, but will catch any misuse
    std::byte* const drawcall_start = mWriter.buffer_head() - sizeof(phi::cmd::draw) * num_drawcalls;
    cc::span<phi::cmd::draw> const drawcalls_as_span = {reinterpret_cast<phi::cmd::draw*>(drawcall_start), num_drawcalls};
    std::sort(drawcalls_as_span.begin(), drawcalls_as_span.end(), [](phi::cmd::draw const& a, phi::cmd::draw const& b) {
        CC_ASSERT(a.s_internal_type == phi::cmd::detail::cmd_type::draw && "draw calls interleaved or different amount recorded");
        return unsigned(a.pipeline_state._value) < unsigned(b.pipeline_state._value);
    });
}

phi::handle::pipeline_state raii::Frame::framebufferAcquireGraphicsPSO(const graphics_pass_info& gp, const framebuffer_info& fb, int fb_inferred_num_samples)
{
    CC_ASSERT(fb_inferred_num_samples == -1
              || fb_inferred_num_samples == gp._storage.get().graphics_config.samples && "graphics_pass_info has incorrect amount of samples configured");
    auto const combined_hash = cc::hash_combine(gp.get_hash(), fb.get_hash());
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
    auto const hash = arg._info.get_xxhash();
    auto const res = mCtx->acquire_graphics_sv(hash, arg._info);
    mFreeables.push_back({freeable_cached_obj::graphics_sv, hash});
    return res;
}

phi::handle::shader_view raii::Frame::passAcquireComputeShaderView(const argument& arg)
{
    auto const hash = arg._info.get_xxhash();
    auto const res = mCtx->acquire_compute_sv(hash, arg._info);
    mFreeables.push_back({freeable_cached_obj::compute_sv, hash});
    return res;
}

void raii::Frame::finalize() { flushPendingTransitions(); }
