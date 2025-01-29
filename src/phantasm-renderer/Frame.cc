#include "Frame.hh"

#include <clean-core/hash_combine.hh>
#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/common/byte_util.hh>
#include <phantasm-hardware-interface/common/format_size.hh>
#include <phantasm-hardware-interface/types.hh>
#include <phantasm-hardware-interface/util.hh>

#include <phantasm-renderer/Context.hh>
#include <phantasm-renderer/common/log.hh>

#include "CompiledFrame.hh"


namespace
{
CC_FORCE_INLINE uint32_t ceil_to_1mb(uint32_t value) { return cc::align_up(value, 1024 * 1024); }
}

pr::raii::Framebuffer pr::raii::Frame::make_framebuffer(const phi::cmd::begin_render_pass& raw_command, bool auto_transition) &
{
    return buildFramebuffer(raw_command, -1, nullptr, auto_transition);
}

pr::raii::Frame& pr::raii::Frame::operator=(raii::Frame&& rhs) noexcept
{
    if (this != &rhs)
    {
        internalDestroy();

        mBackend = rhs.mBackend;
        mList = rhs.mList;
        mPendingTransitionCommand = rhs.mPendingTransitionCommand;
        mFreeables = cc::move(rhs.mFreeables);
        mDeferredFreeResources = cc::move(rhs.mDeferredFreeResources);
        mCacheFreeResources = cc::move(rhs.mCacheFreeResources);
        mCtx = rhs.mCtx;
        mFramebufferActive = rhs.mFramebufferActive;
        mPresentAfterSubmitRequest = rhs.mPresentAfterSubmitRequest;
        rhs.mCtx = nullptr;
    }

    return *this;
}

pr::raii::ComputePass pr::raii::Frame::make_pass(const compute_pass_info& cp) & { return {this, acquireComputePSO(cp)}; }

void pr::raii::Frame::transition(const buffer& res, pr::state target, shader_flags dependency)
{
    auto const& resDesc = mBackend->getResourceBufferDescription(res.handle);
    if (resDesc.heap != resource_heap::gpu) // mapped buffers are never transitioned
        return;

    transition(res.handle, target, dependency);
}

void pr::raii::Frame::transition(const texture& res, pr::state target, shader_flags dependency) { transition(res.handle, target, dependency); }

void pr::raii::Frame::transition(phi::handle::resource raw_resource, pr::state target, shader_flags dependency)
{
    if (mPendingTransitionCommand.transitions.size() == phi::limits::max_resource_transitions)
        flushPendingTransitions();

    mPendingTransitionCommand.add(raw_resource, target, dependency);
}

void pr::raii::Frame::barrier_uav(cc::span<phi::handle::resource const> resources)
{
    phi::cmd::barrier_uav bcmd;
    if (resources.empty())
    {
        // empty UAV barrier list is valid as well
        mBackend->cmdBarrierUAV(mList, bcmd);
        return;
    }

    for (phi::handle::resource const res : resources)
    {
        if (bcmd.resources.full())
        {
            mBackend->cmdBarrierUAV(mList, bcmd);
            bcmd.resources.clear();
        }

        bcmd.resources.push_back(res);
    }

    if (!bcmd.resources.empty())
    {
        // epilogue
        mBackend->cmdBarrierUAV(mList, bcmd);
    }
}

void pr::raii::Frame::present_after_submit(const texture& backbuffer, swapchain sc)
{
    CC_ASSERT(!mPresentAfterSubmitRequest.is_valid() && "only one present_after_submit per pr::raii::Frame allowed");
    transition(backbuffer, state::present);
    mPresentAfterSubmitRequest = sc.handle;
}

void pr::raii::Frame::copy(const buffer& src, const buffer& dest, uint32_t src_offset, uint32_t dest_offset, uint32_t num_bytes)
{
    auto const& srcDesc = mBackend->getResourceBufferDescription(src.handle);
    auto const& destDesc = mBackend->getResourceBufferDescription(dest.handle);

    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    uint32_t const num_copied_bytes = num_bytes > 0 ? num_bytes : cc::min(srcDesc.size_bytes, destDesc.size_bytes);
    CC_ASSERT(num_copied_bytes + src_offset <= srcDesc.size_bytes && num_copied_bytes + dest_offset <= destDesc.size_bytes && "Buffer Copy OOB");

    phi::cmd::copy_buffer ccmd;
    ccmd.init(src.handle, dest.handle, num_copied_bytes, src_offset, dest_offset);

    mBackend->cmdCopyBuffer(mList, ccmd);
}

void pr::raii::Frame::copy(const buffer& src, const texture& dest, uint32_t src_offset, uint32_t dest_mip_index, uint32_t dest_array_index)
{
    auto const& destDesc = mBackend->getResourceTextureDescription(dest.handle);
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();


    phi::cmd::copy_buffer_to_texture ccmd;
    ccmd.init(src.handle, dest.handle, uint32_t(destDesc.width) / (1 + dest_mip_index), uint32_t(destDesc.height) / (1 + dest_mip_index), src_offset,
              dest_mip_index, dest_array_index);

    mBackend->cmdCopyBufferToTexture(mList, ccmd);
}

void pr::raii::Frame::copy(texture const& src, buffer const& dest, uint32_t dest_offset, uint32_t src_mip_index, uint32_t src_array_index)
{
    auto const& srcDesc = mBackend->getResourceTextureDescription(src.handle);
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture_to_buffer ccmd;

    auto const mipWidth = uint32_t(phi::util::get_mip_size(srcDesc.width, src_mip_index));
    auto const mipHeight = uint32_t(phi::util::get_mip_size(srcDesc.height, src_mip_index));
    ccmd.init(src.handle, dest.handle, mipWidth, mipHeight, dest_offset, src_mip_index, src_array_index);

    mBackend->cmdCopyTextureToBuffer(mList, ccmd);
}

void pr::raii::Frame::copy(texture const& src, buffer const& dest, tg::uvec3 src_offset_texels, tg::uvec3 src_extent_pixels, uint32_t dest_offset, uint32_t src_mip_index, uint32_t src_array_index)
{
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture_to_buffer ccmd;
    ccmd.init(src.handle, dest.handle, src_extent_pixels.x, src_extent_pixels.y, dest_offset, src_mip_index, src_array_index);
    ccmd.src_offset_x = src_offset_texels.x;
    ccmd.src_offset_y = src_offset_texels.y;
    ccmd.src_offset_z = src_offset_texels.z;
    ccmd.src_depth = src_extent_pixels.z;
    mBackend->cmdCopyTextureToBuffer(mList, ccmd);
}

void pr::raii::Frame::copy_all_subresources(texture const& src, buffer const& dest)
{
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    auto const& texInfo = context().get_texture_info(src);
    uint32_t const numSlices = texInfo.get_array_size();
    uint32_t const texDepth = texInfo.get_depth();
    bool const bIsD3D12 = context().get_backend_type() == backend::d3d12;

    uint32_t bufferOffset = 0;
    for (uint32_t slice = 0; slice < numSlices; ++slice)
    {
        for (uint32_t mip = 0; mip < texInfo.num_mips; ++mip)
        {
            this->copy(src, dest, bufferOffset, mip, slice);

            auto const subresSizeBytes
                = phi::util::get_texture_subresource_size_bytes_on_gpu(texInfo.fmt, texInfo.width, texInfo.height, texDepth, mip, bIsD3D12);
            bufferOffset += cc::align_up(subresSizeBytes, 512);
        }
    }
}

void pr::raii::Frame::copy(const texture& src, const texture& dest, uint32_t mip_index)
{
    auto const& srcDesc = mBackend->getResourceTextureDescription(src.handle);
    auto const& destDesc = mBackend->getResourceTextureDescription(dest.handle);
    CC_ASSERT(srcDesc.width == destDesc.width && "copy size mismatch");
    CC_ASSERT(srcDesc.height == destDesc.height && "copy size mismatch");
    CC_ASSERT(mip_index < destDesc.num_mips && mip_index < srcDesc.num_mips && "mip index out of bounds");
    copyTextureInternal(src.handle, dest.handle, destDesc.width, destDesc.height, mip_index, 0, destDesc.depth_or_array_size);
}

void pr::raii::Frame::copy_subsection(const texture& src,
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
    mBackend->cmdCopyTexture(mList, ccmd);
}

void pr::raii::Frame::resolve(const texture& src, const texture& dest)
{
    auto const& texDesc = mBackend->getResourceTextureDescription(dest.handle);
    resolveTextureInternal(src.handle, dest.handle, texDesc.width, texDesc.height);
}

void pr::raii::Frame::write_timestamp(const query_range& query_range, uint32_t index)
{
    CC_ASSERT(query_range.type == query_type::timestamp && "Expected timestamp query range to write a timestamp to");
    CC_ASSERT(index < query_range.num && "OOB query write");

    phi::cmd::write_timestamp wcmd;
    wcmd.index = index;
    wcmd.query_range = query_range.handle;
    mBackend->cmdWriteTimestamp(mList, wcmd);
}

void pr::raii::Frame::resolve_queries(const query_range& src, const buffer& dest, uint32_t first_query, uint32_t num_queries, uint32_t dest_offset_bytes)
{
    CC_ASSERT(first_query + num_queries <= src.num && "OOB query resolve read");
    flushPendingTransitions();

    phi::cmd::resolve_queries rcmd;
    rcmd.init(dest.handle, src.handle, first_query, num_queries, dest_offset_bytes);
    mBackend->cmdResolveQueries(mList, rcmd);
}

void pr::raii::Frame::begin_debug_label(char const* label) { mBackend->cmdBeginDebugLabel(mList, phi::cmd::begin_debug_label{label}); }

void pr::raii::Frame::end_debug_label() { mBackend->cmdEndDebugLabel(mList, phi::cmd::end_debug_label{}); }

void pr::raii::Frame::upload_texture_data(cc::span<const std::byte> texture_data, const buffer& upload_buffer, const texture& dest_texture)
{
    auto const& upload_buffer_desc = this->mBackend->getResourceBufferDescription(upload_buffer.handle);
    auto const& dest_texture_desc = this->mBackend->getResourceTextureDescription(dest_texture.handle);
    CC_ASSERT(dest_texture_desc.depth_or_array_size == 1 && "array upload unimplemented");

    transition(dest_texture, pr::state::copy_dest);
    flushPendingTransitions();

    auto const bytes_per_pixel = phi::util::get_format_size_bytes(dest_texture_desc.fmt);
    bool const use_d3d12_per_row_alignment = mCtx->get_backend_type() == pr::backend::d3d12;

    phi::cmd::copy_buffer_to_texture command;
    command.source.buffer = upload_buffer.handle;
    command.destination = dest_texture.handle;
    command.dest_width = uint32_t(dest_texture_desc.width);
    command.dest_height = uint32_t(dest_texture_desc.height);
    command.dest_mip_index = 0;

    std::byte* const upload_buffer_map = mCtx->map_buffer(upload_buffer, 0, 0); // no invalidate
    uint32_t accumulated_offset_bytes = 0u;

    for (auto a = 0u; a < dest_texture_desc.depth_or_array_size; ++a)
    {
        command.dest_array_index = a;
        command.source.offset_bytes = accumulated_offset_bytes;

        mBackend->cmdCopyBufferToTexture(mList, command);

        auto const row_size_bytes = bytes_per_pixel * command.dest_width;
        auto row_stride_bytes = row_size_bytes;

        // texture (pixel) rows are 256-byte aligned per row in d3d12
        if (use_d3d12_per_row_alignment)
        {
            row_stride_bytes = phi::util::align_up(row_stride_bytes, 256);
        }

        auto const offset_bytes = row_stride_bytes * command.dest_height;
        accumulated_offset_bytes += offset_bytes;

        CC_ASSERT(phi::util::is_rowwise_texture_data_copy_in_bounds(              //
                      row_stride_bytes,                                           //
                      row_size_bytes,                                             //
                      command.dest_height,                                        //
                      texture_data.size(),                                        //
                      upload_buffer_desc.size_bytes - command.source.offset_bytes //
                      )
                  && "[Frame::upload_texture_data] source data or destination buffer too small");

        phi::util::copy_texture_data_rowwise(texture_data.data(), upload_buffer_map + command.source.offset_bytes, row_stride_bytes, row_size_bytes,
                                             command.dest_height);
    }

    mCtx->unmap_buffer(upload_buffer); // full flush
}

void pr::raii::Frame::auto_upload_texture_data(cc::span<const std::byte> texture_data, const texture& dest_texture)
{
    // ceil size to 1 MB to make cache hits more likely
    pr::cached_buffer upload_buffer = mCtx->get_upload_buffer(ceil_to_1mb(mCtx->calculate_texture_upload_size(dest_texture, 1u)), 0u);

    upload_texture_data(texture_data, upload_buffer, dest_texture);

    free_to_cache_after_submit(upload_buffer.disown());
}

void pr::raii::Frame::auto_upload_buffer_data(cc::span<std::byte const> data, buffer const& dest_buffer)
{
    // ceil size to 1 MB to make cache hits more likely
    pr::cached_buffer upload_buffer = mCtx->get_upload_buffer(ceil_to_1mb(uint32_t(data.size())), 0u);

    mCtx->write_to_buffer_raw(upload_buffer, data);
    this->copy(upload_buffer, dest_buffer, 0, 0, uint32_t(data.size()));

    free_to_cache_after_submit(upload_buffer.disown());
}

size_t pr::raii::Frame::upload_texture_subresource(cc::span<const std::byte> texture_data,
                                                   uint32_t row_size_bytes,
                                                   const buffer& upload_buffer,
                                                   uint32_t buffer_offset_bytes,
                                                   const texture& dest_texture,
                                                   uint32_t dest_mip_index,
                                                   uint32_t dest_array_index)
{
    auto const& upbufDesc = mBackend->getResourceBufferDescription(upload_buffer.handle);
    auto const& texDesc = mBackend->getResourceTextureDescription(dest_texture.handle);

    flushPendingTransitions();

    tg::isize2 const mip_resolution = phi::util::get_mip_size({texDesc.width, texDesc.height}, dest_mip_index);

    phi::cmd::copy_buffer_to_texture command;
    command.source.buffer = upload_buffer.handle;
    command.source.offset_bytes = buffer_offset_bytes;
    command.destination = dest_texture.handle;
    command.dest_width = uint32_t(mip_resolution.width);
    command.dest_height = uint32_t(mip_resolution.height);
    command.dest_mip_index = dest_mip_index;
    command.dest_array_index = dest_array_index;
    mBackend->cmdCopyBufferToTexture(mList, command);

    bool const use_d3d12_per_row_alignment = mCtx->get_backend_type() == pr::backend::d3d12;
    auto row_stride_bytes = row_size_bytes;

    // texture (pixel) rows are 256-byte aligned per row in d3d12
    if (use_d3d12_per_row_alignment)
    {
        row_stride_bytes = phi::util::align_up(row_stride_bytes, 256);
    }

    std::byte* const upload_buffer_map = mCtx->map_buffer(upload_buffer, 0, 0); // no invalidate

    auto const num_rows = uint32_t(texture_data.size() / row_size_bytes);

    CC_ASSERT(phi::util::is_rowwise_texture_data_copy_in_bounds(row_stride_bytes, row_size_bytes, num_rows, uint32_t(texture_data.size()),
                                                                upbufDesc.size_bytes - command.source.offset_bytes)
              && "[Frame::upload_texture_subresource] source data or destination buffer too small");

    auto const last_offset = phi::util::copy_texture_data_rowwise(texture_data.data(), upload_buffer_map + command.source.offset_bytes,
                                                                  row_stride_bytes, row_size_bytes, num_rows);

    mCtx->unmap_buffer(upload_buffer, int32_t(buffer_offset_bytes), int32_t(buffer_offset_bytes + last_offset)); // flush exact range

    // amount of bytes written to upload buffer, starting at offset
    return last_offset;
}

void pr::raii::Frame::transition_slices(cc::span<const phi::cmd::transition_image_slices::slice_transition_info> slices)
{
    flushPendingTransitions();

    phi::cmd::transition_image_slices tcmd;
    for (auto const& ti : slices)
    {
        if (tcmd.transitions.full())
        {
            mBackend->cmdTransitionImageSlices(mList, tcmd);
            tcmd.transitions.clear();
        }

        tcmd.transitions.push_back(ti);
    }

    if (!tcmd.transitions.empty())
    {
        mBackend->cmdTransitionImageSlices(mList, tcmd);
    }
}

void pr::raii::Frame::transition_slices(phi::cmd::transition_image_slices const& tcmd)
{
    flushPendingTransitions();
    mBackend->cmdTransitionImageSlices(mList, tcmd);
}

void pr::raii::Frame::addRenderTargetToFramebuffer(phi::cmd::begin_render_pass& bcmd, int& num_samples, const texture& rt)
{
    auto const& texDesc = mBackend->getResourceTextureDescription(rt.handle);
    bcmd.viewport.width = cc::min(bcmd.viewport.width, texDesc.width);
    bcmd.viewport.height = cc::min(bcmd.viewport.height, texDesc.height);

    if (num_samples != -1)
    {
        CC_ASSERT(num_samples == int(texDesc.num_samples) && "make_framebuffer: inconsistent amount of samples in render targets");
    }

    if (phi::util::is_depth_format(texDesc.fmt))
    {
        CC_ASSERT(!bcmd.depth_target.rv.resource.is_valid() && "passed multiple depth targets to pr::raii::Frame::make_framebuffer");
        bcmd.set_2d_depth_stencil(rt.handle, texDesc.fmt, phi::rt_clear_type::clear, texDesc.num_samples > 1);
    }
    else
    {
        bcmd.add_2d_rt(rt.handle, texDesc.fmt, phi::rt_clear_type::clear, texDesc.num_samples > 1);
    }
}

void pr::raii::Frame::flushPendingTransitions()
{
    if (!mPendingTransitionCommand.transitions.empty())
    {
        CC_ASSERT(!mFramebufferActive && "No transitions allowed during active pr::raii::Framebuffers");
        mBackend->cmdTransitionResources(mList, mPendingTransitionCommand);
        mPendingTransitionCommand.transitions.clear();
    }
}

void pr::raii::Frame::copyTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h, uint32_t mip_index, uint32_t first_array_index, uint32_t num_array_slices)
{
    transition(src, pr::state::copy_src);
    transition(dest, pr::state::copy_dest);
    flushPendingTransitions();

    phi::cmd::copy_texture ccmd;
    ccmd.init_symmetric(src, dest, uint32_t(w), uint32_t(h), mip_index, first_array_index, num_array_slices);
    mBackend->cmdCopyTexture(mList, ccmd);
}

void pr::raii::Frame::resolveTextureInternal(phi::handle::resource src, phi::handle::resource dest, int w, int h)
{
    transition(src, pr::state::resolve_src);
    transition(dest, pr::state::resolve_dest);
    flushPendingTransitions();

    phi::cmd::resolve_texture ccmd;
    ccmd.init_symmetric(src, dest, uint32_t(w), uint32_t(h));
    mBackend->cmdResolveTexture(mList, ccmd);
}

phi::handle::pipeline_state pr::raii::Frame::acquireComputePSO(const compute_pass_info& cp)
{
    auto const cp_hash = cp.get_hash();
    auto const res = mCtx->acquire_compute_pso(cp_hash, cp);
    mFreeables.push_back({freeable_cached_obj::compute_pso, cp_hash});
    return res;
}

void pr::raii::Frame::internalDestroy()
{
    if (mCtx != nullptr)
    {
        mCtx->free_all(mFreeables); // usually this is empty from a move during Context::compile(Frame&)
        mCtx = nullptr;
    }
}

pr::raii::Framebuffer pr::raii::Frame::buildFramebuffer(const phi::cmd::begin_render_pass& bcmd,
                                                        int num_samples,
                                                        const phi::arg::framebuffer_config* blendstate_override,
                                                        bool auto_transition)
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
    mBackend->cmdBeginRenderPass(mList, bcmd);

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
        fb_info._storage.logic_op_enable = blendstate_override->logic_op_enable;
        fb_info._storage.logic_op = blendstate_override->logic_op;

        CC_ASSERT(fb_info._storage.render_targets.size() == blendstate_override->render_targets.size() && "inconsistent blendstate override");

        for (uint8_t i = 0u; i < blendstate_override->render_targets.size(); ++i)
        {
            if (blendstate_override->render_targets[i].blend_enable)
            {
                auto& target_rt = fb_info._storage.render_targets[i];

                target_rt.blend_enable = true;
                target_rt.state = blendstate_override->render_targets[i].state;
            }
        }
    }

    return {this, fb_info, num_samples, has_blendstate_overrides};
}

void pr::raii::Frame::framebufferOnJoin(const pr::raii::Framebuffer&)
{
    phi::cmd::end_render_pass ecmd;

    mBackend->cmdEndRenderPass(mList, ecmd);
    mFramebufferActive = false;
}

phi::handle::pipeline_state pr::raii::Frame::framebufferAcquireGraphicsPSO(const graphics_pass_info& gp, const framebuffer_info& fb, int fb_inferred_num_samples)
{
    CC_ASSERT((fb_inferred_num_samples == -1 || fb_inferred_num_samples == gp._storage.get().graphics_config.samples)
              && "graphics_pass_info has incorrect amount of samples configured");
    auto const combined_hash = cc::hash_combine(gp.get_hash(), fb.get_hash());
    auto const res = mCtx->acquire_graphics_pso(combined_hash, gp, fb);
    mFreeables.push_back({freeable_cached_obj::graphics_pso, combined_hash});
    return res;
}

void pr::raii::Frame::passOnDispatch(const phi::cmd::dispatch& dcmd)
{
    flushPendingTransitions();
    mBackend->cmdDispatch(mList, dcmd);
}

phi::handle::shader_view pr::raii::Frame::passAcquireShaderView(cc::span<view> srvs, cc::span<view> uavs, cc::span<phi::sampler_config const> samplers, bool compute)
{
    fixup_incomplete_views(mCtx, srvs, uavs);

    uint64_t hash = 0;
    auto const res = mCtx->acquire_shader_view(compute, &hash, srvs, uavs, samplers);

    mFreeables.push_back({compute ? freeable_cached_obj::compute_sv : freeable_cached_obj::graphics_sv, hash});

    return res;
}

void pr::raii::Frame::finalize() { flushPendingTransitions(); }

pr::raii::Frame::Frame(pr::Context* ctx, phi::handle::live_command_list list, cc::allocator* alloc)
  : mBackend(&ctx->get_backend()), mList(list), mFreeables(alloc), mDeferredFreeResources(alloc), mCacheFreeResources(alloc), mCtx(ctx)
{
}

pr::raii::Frame::Frame(Frame&& rhs) noexcept
  : mBackend(rhs.mBackend),
    mList(rhs.mList),
    mPendingTransitionCommand(rhs.mPendingTransitionCommand),
    mFreeables(cc::move(rhs.mFramebufferActive)),
    mDeferredFreeResources(cc::move(rhs.mDeferredFreeResources)),
    mCacheFreeResources(cc::move(rhs.mCacheFreeResources)),
    mCtx(rhs.mCtx),
    mFramebufferActive(rhs.mFramebufferActive),
    mPresentAfterSubmitRequest(rhs.mPresentAfterSubmitRequest)
{
    rhs.mCtx = nullptr;
}


void pr::raii::Frame::begin_profile_scope(phi::cmd::begin_profile_scope const& bps) { mBackend->cmdBeginProfileScope(mList, bps); }

void pr::raii::Frame::end_profile_scope() { mBackend->cmdEndProfileScope(mList, phi::cmd::end_profile_scope{}); }

void pr::raii::Frame::raw_draw(phi::cmd::draw const& dcmd)
{
    flushPendingTransitions();
    mBackend->cmdDraw(mList, dcmd);
}

void pr::raii::Frame::raw_draw_indirect(phi::cmd::draw_indirect const& dcmd)
{
    flushPendingTransitions();
    mBackend->cmdDrawIndirect(mList, dcmd);
}

void pr::raii::Frame::raw_dipatch(phi::cmd::dispatch const& dcmd)
{
    flushPendingTransitions();
    mBackend->cmdDispatch(mList, dcmd);
}

void pr::raii::Frame::raw_dipatch_indirect(phi::cmd::dispatch_indirect const& dcmd)
{
    flushPendingTransitions();
    mBackend->cmdDispatchIndirect(mList, dcmd);
}

void pr::raii::Frame::raw_dispatch_rays(phi::cmd::dispatch_rays const& dcmd)
{
    flushPendingTransitions();
    mBackend->cmdDispatchRays(mList, dcmd);
}

void pr::raii::Frame::raw_clear_textures(phi::cmd::clear_textures const& ccmd)
{
    flushPendingTransitions();
    mBackend->cmdClearTextures(mList, ccmd);
}
