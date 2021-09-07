#include "Frame.hh"

#include <algorithm> // std::sort

#include <clean-core/hash_combine.hh>
#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/common/byte_util.hh>
#include <phantasm-hardware-interface/common/format_size.hh>
#include <phantasm-hardware-interface/util.hh>

#include <phantasm-renderer/Context.hh>
#include <phantasm-renderer/common/log.hh>

#include "CompiledFrame.hh"


namespace
{
bool is_rowwise_copy_in_bounds(size_t dest_row_stride_bytes, size_t row_size_bytes, size_t num_rows, size_t source_size_bytes, size_t destination_size_bytes)
{
    // num_rows is the height in pixels for regular formats, but is lower for block compressed formats
    auto const largest_src_access = num_rows * row_size_bytes;
    auto const largest_dest_access = (num_rows - 1) * dest_row_stride_bytes + row_size_bytes;

    bool const is_in_bounds = (largest_src_access <= source_size_bytes) && (largest_dest_access <= destination_size_bytes);

    if (!is_in_bounds)
    {
        PR_LOG_ERROR("Frame::upload_texture_data: rowwise copy from texture data to upload buffer is out of bounds");
        if (largest_src_access > source_size_bytes)
            PR_LOG_ERROR("src bound error: access {} > size {} (exceeding by {} B)", largest_src_access, source_size_bytes, largest_src_access - source_size_bytes);
        if (largest_dest_access > destination_size_bytes)
            PR_LOG_ERROR("dst bound error: access {} > size {} (exceeding by {} B)", largest_dest_access, destination_size_bytes,
                         largest_dest_access - destination_size_bytes);

        PR_LOG_ERROR("while writing {} rows of {} bytes (strided {})", num_rows, row_size_bytes, dest_row_stride_bytes);
    }

    return is_in_bounds;
}

// returns bytes written to dest
size_t rowwise_copy(std::byte const* __restrict src, std::byte* __restrict dest, size_t dest_row_stride_bytes, size_t row_size_bytes, uint32_t num_rows)
{
    // num_rows is the height in pixels for regular formats, but is lower for block compressed formats
    for (auto y = 0u; y < num_rows; ++y)
    {
        auto const src_offset = y * row_size_bytes;
        auto const dst_offset = y * dest_row_stride_bytes;
        std::memcpy(dest + dst_offset, src + src_offset, row_size_bytes);
    }

    return dest_row_stride_bytes * (num_rows - 1) + row_size_bytes;
}

CC_FORCE_INLINE uint32_t ceil_to_1mb(uint32_t value) { return cc::align_up(value, 1024 * 1024); }
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
    auto const& resDesc = mCtx->get_backend().getResourceBufferDescription(res.handle);
    if (resDesc.heap != resource_heap::gpu) // mapped buffers are never transitioned
        return;

    transition(res.handle, target, dependency);
}

void raii::Frame::transition(const texture& res, pr::state target, shader_flags dependency) { transition(res.handle, target, dependency); }

void raii::Frame::transition(phi::handle::resource raw_resource, pr::state target, shader_flags dependency)
{
    if (mPendingTransitionCommand.transitions.size() == phi::limits::max_resource_transitions)
        flushPendingTransitions();

    mPendingTransitionCommand.add(raw_resource, target, dependency);
}

void pr::raii::Frame::barrier_uav(cc::span<phi::handle::resource const> resources)
{
    phi::cmd::barrier_uav bcmd;

    for (phi::handle::resource const res : resources)
    {
        if (bcmd.resources.full())
        {
            write_raw_cmd(bcmd);
            bcmd.resources.clear();
        }

        bcmd.resources.push_back(res);
    }

    // empty UAV barrier list is valid as well
    write_raw_cmd(bcmd);
}

void raii::Frame::present_after_submit(const texture& backbuffer, swapchain sc)
{
    CC_ASSERT(!mPresentAfterSubmitRequest.is_valid() && "only one present_after_submit per pr::raii::Frame allowed");
    transition(backbuffer, state::present);
    mPresentAfterSubmitRequest = sc.handle;
}

void raii::Frame::copy(const buffer& src, const buffer& dest, size_t src_offset, size_t dest_offset, size_t num_bytes)
{
    auto const& srcDesc = mCtx->get_backend().getResourceBufferDescription(src.handle);
    auto const& destDesc = mCtx->get_backend().getResourceBufferDescription(dest.handle);

    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    size_t const num_copied_bytes = num_bytes > 0 ? num_bytes : cc::min(srcDesc.size_bytes, destDesc.size_bytes);
    CC_ASSERT(num_copied_bytes + src_offset <= srcDesc.size_bytes && num_copied_bytes + dest_offset <= destDesc.size_bytes && "Buffer Copy OOB");

    phi::cmd::copy_buffer ccmd;
    ccmd.init(src.handle, dest.handle, num_copied_bytes, src_offset, dest_offset);
    mWriter.add_command(ccmd);
}

void raii::Frame::copy(const buffer& src, const texture& dest, size_t src_offset, uint32_t dest_mip_index, uint32_t dest_array_index)
{
    auto const& destDesc = mCtx->get_backend().getResourceTextureDescription(dest.handle);
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();


    phi::cmd::copy_buffer_to_texture ccmd;
    ccmd.init(src.handle, dest.handle, uint32_t(destDesc.width) / (1 + dest_mip_index), uint32_t(destDesc.height) / (1 + dest_mip_index), src_offset,
              dest_mip_index, dest_array_index);
    mWriter.add_command(ccmd);
}

void pr::raii::Frame::copy(texture const& src, buffer const& dest, size_t dest_offset, uint32_t src_mip_index, uint32_t src_array_index)
{
    auto const& srcDesc = mCtx->get_backend().getResourceTextureDescription(src.handle);
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture_to_buffer ccmd;

    auto const mipWidth = uint32_t(phi::util::get_mip_size(srcDesc.width, src_mip_index));
    auto const mipHeight = uint32_t(phi::util::get_mip_size(srcDesc.height, src_mip_index));
    ccmd.init(src.handle, dest.handle, mipWidth, mipHeight, dest_offset, src_mip_index, src_array_index);
    mWriter.add_command(ccmd);
}

void raii::Frame::copy(const texture& src, const texture& dest, uint32_t mip_index)
{
    auto const& srcDesc = mCtx->get_backend().getResourceTextureDescription(src.handle);
    auto const& destDesc = mCtx->get_backend().getResourceTextureDescription(dest.handle);
    CC_ASSERT(srcDesc.width == destDesc.width && "copy size mismatch");
    CC_ASSERT(srcDesc.height == destDesc.height && "copy size mismatch");
    CC_ASSERT(mip_index < destDesc.num_mips && mip_index < srcDesc.num_mips && "mip index out of bounds");
    copyTextureInternal(src.handle, dest.handle, destDesc.width, destDesc.height, mip_index, 0, destDesc.depth_or_array_size);
}

void raii::Frame::copy_subsection(const texture& src,
                                  const texture& dest,
                                  uint32_t src_mip_index,
                                  uint32_t src_array_index,
                                  uint32_t dest_mip_index,
                                  uint32_t dest_array_index,
                                  uint32_t num_array_slices,
                                  tg::isize2 dest_size)
{
    // TODO: OOB asserts
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture ccmd;
    ccmd.source = src.handle;
    ccmd.destination = src.handle;
    ccmd.src_mip_index = src_mip_index;
    ccmd.src_array_index = src_array_index;
    ccmd.dest_mip_index = dest_mip_index;
    ccmd.dest_array_index = dest_array_index;
    ccmd.width = uint32_t(dest_size.width);
    ccmd.height = uint32_t(dest_size.height);
    ccmd.num_array_slices = num_array_slices;
    mWriter.add_command(ccmd);
}

void raii::Frame::resolve(const texture& src, const texture& dest)
{
    auto const& texDesc = mCtx->get_backend().getResourceTextureDescription(dest.handle);
    resolveTextureInternal(src.handle, dest.handle, texDesc.width, texDesc.height);
}

void raii::Frame::write_timestamp(const query_range& query_range, uint32_t index)
{
    CC_ASSERT(query_range.type == query_type::timestamp && "Expected timestamp query range to write a timestamp to");
    CC_ASSERT(index < query_range.num && "OOB query write");

    phi::cmd::write_timestamp wcmd;
    wcmd.index = index;
    wcmd.query_range = query_range.handle;
    mWriter.add_command(wcmd);
}

void raii::Frame::resolve_queries(const query_range& src, const buffer& dest, uint32_t first_query, uint32_t num_queries, uint32_t dest_offset_bytes)
{
    CC_ASSERT(first_query + num_queries <= src.num && "OOB query resolve read");
    flushPendingTransitions();

    phi::cmd::resolve_queries rcmd;
    rcmd.init(dest.handle, src.handle, first_query, num_queries, dest_offset_bytes);
    mWriter.add_command(rcmd);
}

void raii::Frame::upload_texture_data(cc::span<const std::byte> texture_data, const buffer& upload_buffer, const texture& dest_texture)
{
    auto const& upload_buffer_desc = this->mCtx->get_backend().getResourceBufferDescription(upload_buffer.handle);
    auto const& dest_texture_desc = this->mCtx->get_backend().getResourceTextureDescription(dest_texture.handle);
    CC_ASSERT(dest_texture_desc.depth_or_array_size == 1 && "array upload unimplemented");

    transition(dest_texture, pr::state::copy_dest);
    flushPendingTransitions();

    auto const bytes_per_pixel = phi::util::get_format_size_bytes(dest_texture_desc.fmt);
    bool const use_d3d12_per_row_alignment = mCtx->get_backend_type() == pr::backend::d3d12;

    phi::cmd::copy_buffer_to_texture command;
    command.source = upload_buffer.handle;
    command.destination = dest_texture.handle;
    command.dest_width = uint32_t(dest_texture_desc.width);
    command.dest_height = uint32_t(dest_texture_desc.height);
    command.dest_mip_index = 0;

    std::byte* const upload_buffer_map = mCtx->map_buffer(upload_buffer, 0, 0); // no invalidate
    size_t accumulated_offset_bytes = 0u;

    for (auto a = 0u; a < dest_texture_desc.depth_or_array_size; ++a)
    {
        command.dest_array_index = a;
        command.source_offset_bytes = accumulated_offset_bytes;

        mWriter.add_command(command);

        auto const row_size_bytes = bytes_per_pixel * command.dest_width;
        auto row_stride_bytes = row_size_bytes;

        // texture (pixel) rows are 256-byte aligned per row in d3d12
        if (use_d3d12_per_row_alignment)
        {
            row_stride_bytes = phi::util::align_up(row_stride_bytes, 256);
        }

        auto const offset_bytes = row_stride_bytes * command.dest_height;
        accumulated_offset_bytes += offset_bytes;

        CC_ASSERT(is_rowwise_copy_in_bounds(row_stride_bytes, row_size_bytes, command.dest_height, texture_data.size(),
                                            upload_buffer_desc.size_bytes - command.source_offset_bytes)
                  && "[Frame::upload_texture_data] source data or destination buffer too small");

        rowwise_copy(texture_data.data(), upload_buffer_map + command.source_offset_bytes, row_stride_bytes, row_size_bytes, command.dest_height);
    }

    mCtx->unmap_buffer(upload_buffer); // full flush
}

void raii::Frame::auto_upload_texture_data(cc::span<const std::byte> texture_data, const texture& dest_texture)
{
    // ceil size to 1 MB to make cache hits more likely
    pr::cached_buffer upload_buffer = mCtx->get_upload_buffer(ceil_to_1mb(mCtx->calculate_texture_upload_size(dest_texture, 1u)), 0u);

    upload_texture_data(texture_data, upload_buffer, dest_texture);

    free_to_cache_after_submit(upload_buffer.disown());
}

void raii::Frame::auto_upload_buffer_data(cc::span<std::byte const> data, buffer const& dest_buffer)
{
    // ceil size to 1 MB to make cache hits more likely
    pr::cached_buffer upload_buffer = mCtx->get_upload_buffer(ceil_to_1mb(uint32_t(data.size())), 0u);

    mCtx->write_to_buffer_raw(upload_buffer, data);
    this->copy(upload_buffer, dest_buffer, 0, 0, data.size());

    free_to_cache_after_submit(upload_buffer.disown());
}

size_t raii::Frame::upload_texture_subresource(cc::span<const std::byte> texture_data,
                                               uint32_t row_size_bytes,
                                               const buffer& upload_buffer,
                                               uint32_t buffer_offset_bytes,
                                               const texture& dest_texture,
                                               uint32_t dest_mip_index,
                                               uint32_t dest_array_index)
{
    auto const& upbufDesc = mCtx->get_backend().getResourceBufferDescription(upload_buffer.handle);
    auto const& texDesc = mCtx->get_backend().getResourceTextureDescription(dest_texture.handle);

    flushPendingTransitions();

    tg::isize2 const mip_resolution = phi::util::get_mip_size({texDesc.width, texDesc.height}, dest_mip_index);

    phi::cmd::copy_buffer_to_texture command;
    command.source = upload_buffer.handle;
    command.destination = dest_texture.handle;
    command.source_offset_bytes = buffer_offset_bytes;
    command.dest_width = uint32_t(mip_resolution.width);
    command.dest_height = uint32_t(mip_resolution.height);
    command.dest_mip_index = dest_mip_index;
    command.dest_array_index = dest_array_index;
    mWriter.add_command(command);

    bool const use_d3d12_per_row_alignment = mCtx->get_backend_type() == pr::backend::d3d12;
    auto row_stride_bytes = row_size_bytes;

    // texture (pixel) rows are 256-byte aligned per row in d3d12
    if (use_d3d12_per_row_alignment)
    {
        row_stride_bytes = phi::util::align_up(row_stride_bytes, 256);
    }

    std::byte* const upload_buffer_map = mCtx->map_buffer(upload_buffer, 0, 0); // no invalidate

    auto const num_rows = uint32_t(texture_data.size() / row_size_bytes);

    CC_ASSERT(is_rowwise_copy_in_bounds(row_stride_bytes, row_size_bytes, num_rows, uint32_t(texture_data.size()),
                                        upbufDesc.size_bytes - uint32_t(command.source_offset_bytes))
              && "[Frame::upload_texture_subresource] source data or destination buffer too small");

    auto const last_offset = rowwise_copy(texture_data.data(), upload_buffer_map + command.source_offset_bytes, row_stride_bytes, row_size_bytes, num_rows);

    mCtx->unmap_buffer(upload_buffer, int32_t(buffer_offset_bytes), int32_t(buffer_offset_bytes + last_offset)); // flush exact range

    // amount of bytes written to upload buffer, starting at offset
    return last_offset;
}

void raii::Frame::transition_slices(cc::span<const phi::cmd::transition_image_slices::slice_transition_info> slices)
{
    flushPendingTransitions();

    phi::cmd::transition_image_slices tcmd;
    for (auto const& ti : slices)
    {
        if (tcmd.transitions.full())
        {
            mWriter.add_command(tcmd);
            tcmd.transitions.clear();
        }

        tcmd.transitions.push_back(ti);
    }

    if (!tcmd.transitions.empty())
        mWriter.add_command(tcmd);
}

void raii::Frame::addRenderTargetToFramebuffer(phi::cmd::begin_render_pass& bcmd, int& num_samples, const texture& rt)
{
    auto const& texDesc = mCtx->get_backend().getResourceTextureDescription(rt.handle);
    bcmd.viewport.width = cc::min(bcmd.viewport.width, texDesc.width);
    bcmd.viewport.height = cc::min(bcmd.viewport.height, texDesc.height);

    if (num_samples != -1)
    {
        CC_ASSERT(num_samples == int(texDesc.num_samples) && "make_framebuffer: inconsistent amount of samples in render targets");
    }

    if (phi::util::is_depth_format(texDesc.fmt))
    {
        CC_ASSERT(!bcmd.depth_target.rv.resource.is_valid() && "passed multiple depth targets to raii::Frame::make_framebuffer");
        bcmd.set_2d_depth_stencil(rt.handle, texDesc.fmt, phi::rt_clear_type::clear, texDesc.num_samples > 1);
    }
    else
    {
        bcmd.add_2d_rt(rt.handle, texDesc.fmt, phi::rt_clear_type::clear, texDesc.num_samples > 1);
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

void raii::Frame::copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h, uint32_t mip_index, uint32_t first_array_index, uint32_t num_array_slices)
{
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture ccmd;
    ccmd.init_symmetric(src, dest, uint32_t(w), uint32_t(h), mip_index, first_array_index, num_array_slices);
    mWriter.add_command(ccmd);
}

void raii::Frame::resolveTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h)
{
    transition(src, pr::state::resolve_src);
    transition(dest, pr::state::resolve_dest);
    flushPendingTransitions();

    phi::cmd::resolve_texture ccmd;
    ccmd.init_symmetric(src, dest, uint32_t(w), uint32_t(h));
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
    }

    flushPendingTransitions();

    mFramebufferActive = true;
    mWriter.add_command(bcmd);

    bool const has_blendstate_overrides = blendstate_override != nullptr;
    // construct framebuffer info for future cache access inside of Framebuffer
    framebuffer_info fb_info;

    for (auto const& target : bcmd.render_targets)
        fb_info.target(target.rv.texture_info.pixel_format);

    if (bcmd.depth_target.rv.resource.is_valid())
        fb_info.depth(bcmd.depth_target.rv.texture_info.pixel_format);

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

void raii::Frame::framebufferOnSortByPSO(uint32_t num_drawcalls)
{
    // sort previously recorded draw commands by PSO, directly in the command buffer
    // requires strictly NOTHING but drawcalls recorded, but will catch any misuse
    std::byte* const drawcall_start = mWriter.buffer_head() - sizeof(phi::cmd::draw) * num_drawcalls;
    cc::span<phi::cmd::draw> const drawcalls_as_span = {reinterpret_cast<phi::cmd::draw*>(drawcall_start), num_drawcalls};
    std::sort(drawcalls_as_span.begin(), drawcalls_as_span.end(),
              [](phi::cmd::draw const& a, phi::cmd::draw const& b)
              {
                  CC_ASSERT(a.s_internal_type == phi::cmd::detail::cmd_type::draw && "draw calls interleaved or different amount recorded");
                  return uint32_t(a.pipeline_state._value) < uint32_t(b.pipeline_state._value);
              });
}

phi::handle::pipeline_state raii::Frame::framebufferAcquireGraphicsPSO(const graphics_pass_info& gp, const framebuffer_info& fb, int fb_inferred_num_samples)
{
    CC_ASSERT((fb_inferred_num_samples == -1 || fb_inferred_num_samples == gp._storage.get().graphics_config.samples)
              && "graphics_pass_info has incorrect amount of samples configured");
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

phi::handle::shader_view raii::Frame::passAcquireGraphicsShaderView(argument& arg)
{
    arg._fixup_incomplete_resource_views(mCtx);
    auto const hash = arg._info.get_xxhash();
    auto const res = mCtx->acquire_graphics_sv(hash, arg._info);
    mFreeables.push_back({freeable_cached_obj::graphics_sv, hash});
    return res;
}

phi::handle::shader_view raii::Frame::passAcquireComputeShaderView(argument& arg)
{
    arg._fixup_incomplete_resource_views(mCtx);
    auto const hash = arg._info.get_xxhash();
    auto const res = mCtx->acquire_compute_sv(hash, arg._info);
    mFreeables.push_back({freeable_cached_obj::compute_sv, hash});
    return res;
}

void raii::Frame::finalize() { flushPendingTransitions(); }

void raii::Frame::consume_other_frame(Frame& other)
{
    // copy raw commands
    other.finalize();
    std::byte* const writePtr = write_raw_bytes(other.mWriter.size());
    std::memcpy(writePtr, other.mWriter.buffer(), other.mWriter.size());
    other.mWriter.reset();

    // copy and consume free-requested items
    mFreeables.push_back_range(other.mFreeables);
    mDeferredFreeResources.push_back_range(other.mDeferredFreeResources);
    mCacheFreeResources.push_back_range(other.mCacheFreeResources);
    other.mFreeables.clear();
    other.mDeferredFreeResources.clear();
    other.mCacheFreeResources.clear();

    // overtake present request
    if (other.mPresentAfterSubmitRequest.is_valid())
    {
        mPresentAfterSubmitRequest = other.mPresentAfterSubmitRequest;
        other.mPresentAfterSubmitRequest.invalidate();
    }
}
