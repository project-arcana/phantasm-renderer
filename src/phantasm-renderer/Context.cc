#include "Context.hh"

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/config.hh>

#include <phantasm-renderer/CompiledFrame.hh>
#include <phantasm-renderer/Frame.hh>
#include <phantasm-renderer/backends.hh>

using namespace pr;

Frame Context::make_frame(size_t initial_size)
{
    // TODO: pool the memory blocks for frames
    return {this, initial_size};
}

CompiledFrame Context::compile(const Frame& frame)
{
    if (frame.isEmpty())
    {
        return CompiledFrame(phi::handle::null_command_list, phi::handle::null_event);
    }
    else
    {
        auto const event = mGpuEpochTracker.get_event();
        auto const cmdlist = mBackend->recordCommandList(frame.getMemory(), frame.getSize(), event);
        return CompiledFrame(cmdlist, event);
    }
}

void Context::submit(const Frame& frame) { submit(compile(frame)); }

void Context::submit(const CompiledFrame& frame)
{
    if (!frame.getCmdlist().is_valid())
        return;

    {
        auto const lg = std::lock_guard(mMutexSubmission);
        auto const cmdlist = frame.getCmdlist();
        mBackend->submit(cc::span{cmdlist});
    }
    mGpuEpochTracker.on_event_submission(frame.getEvent());
}

Context::Context(phi::window_handle const& window_handle)
{
    phi::backend_config cfg;
#ifndef CC_RELEASE
    cfg.validation = phi::validation_level::on_extended;
#endif
    mBackend = make_vulkan_backend(window_handle, cfg);
    initialize();
}

Context::Context(cc::poly_unique_ptr<phi::Backend> backend) : mBackend(cc::move(backend)) { initialize(); }

void Context::initialize()
{
    mGpuEpochTracker.initialize(mBackend.get(), 2048);
    mCacheBuffers.reserve(256);
    mCacheTextures.reserve(256);
    mCacheRenderTargets.reserve(64);
}

phi::arg::shader_binary Context::compileShader(cc::string_view code, phi::shader_stage stage)
{
    CC_RUNTIME_ASSERT(false && "unimplemented");
    return {};
}

phi::handle::resource Context::acquireRenderTarget(const render_target_info& info)
{
    auto const lookup = mCacheRenderTargets.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.is_valid())
    {
        return lookup;
    }
    else
    {
        return mBackend->createRenderTarget(info.format, {info.width, info.height}, info.num_samples);
    }
}

phi::handle::resource Context::acquireTexture(const texture_info& info)
{
    auto const lookup = mCacheTextures.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.is_valid())
    {
        return lookup;
    }
    else
    {
        return mBackend->createTexture(info.format, {info.width, info.height}, info.num_mips, info.dim, info.depth_or_array_size, info.allow_uav);
    }
}

phi::handle::resource Context::acquireBuffer(const buffer_info& info)
{
    auto const lookup = mCacheBuffers.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.is_valid())
    {
        return lookup;
    }
    else
    {
        if (info.is_mapped)
        {
            return mBackend->createMappedBuffer(info.size_bytes, info.stride_bytes);
        }
        else
        {
            return mBackend->createBuffer(info.size_bytes, info.stride_bytes, info.allow_uav);
        }
    }
}

phi::handle::pipeline_state Context::acquirePSO(phi::arg::vertex_format const& vertex_fmt,
                                                phi::arg::framebuffer_config const& fb_conf,
                                                phi::arg::shader_arg_shapes arg_shapes,
                                                bool has_root_consts,
                                                phi::arg::graphics_shaders shaders,
                                                phi::pipeline_config const& pipeline_conf)
{
    // TODO: cache
    LOG(warning)("PSO caching unimplemented");
    return mBackend->createPipelineState(vertex_fmt, fb_conf, arg_shapes, has_root_consts, shaders, pipeline_conf);
}

Context::~Context()
{
    mBackend->flushGPU();

    mCacheTextures.free_all(mBackend.get());
    mCacheRenderTargets.free_all(mBackend.get());
    mCacheBuffers.free_all(mBackend.get());

    mGpuEpochTracker.destroy();
}

void Context::freeRenderTarget(const render_target_info& info, phi::handle::resource res, uint64_t guid)
{
    mCacheRenderTargets.free(res, guid, info, mGpuEpochTracker.get_current_epoch_cpu());
}

void Context::freeTexture(const texture_info& info, phi::handle::resource res, uint64_t guid)
{
    mCacheTextures.free(res, guid, info, mGpuEpochTracker.get_current_epoch_cpu());
}

void Context::freeBuffer(const buffer_info& info, phi::handle::resource res, uint64_t guid)
{
    mCacheBuffers.free(res, guid, info, mGpuEpochTracker.get_current_epoch_cpu());
}


Buffer<untyped_tag> pr::Context::make_untyped_buffer(size_t size, size_t stride, bool read_only)
{
    auto const info = buffer_info{size, stride, !read_only, false};
    return {this, acquireGuid(), acquireBuffer(info), info};
}

Buffer<untyped_tag> pr::Context::make_untyped_upload_buffer(size_t size, size_t stride, bool read_only)
{
    auto const info = buffer_info{size, stride, !read_only, true};
    auto const handle = acquireBuffer(info);
    return {this, acquireGuid(), handle, info, mBackend->getMappedMemory(handle)};
}
