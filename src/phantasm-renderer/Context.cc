#include "Context.hh"

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/config.hh>

#include <phantasm-renderer/CompiledFrame.hh>
#include <phantasm-renderer/Frame.hh>
#include <phantasm-renderer/backends.hh>

using namespace pr;

Frame Context::make_frame()
{
    return {}; // TODO
}

CompiledFrame Context::compile(const Frame& frame)
{
    auto const event = mGpuEpochTracker.get_event();
    auto const cmdlist = mBackend->recordCommandList(nullptr, 0, event);
    return CompiledFrame(cmdlist, event);
}

void Context::submit(const Frame& frame) { submit(compile(frame)); }

void Context::submit(const CompiledFrame& frame)
{
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

Context::Context(cc::poly_unique_ptr<phi::Backend> backend) : mBackend(std::move(backend)) { initialize(); }

void Context::initialize()
{
    mGpuEpochTracker.initialize(mBackend.get(), 2048);
    mCacheBuffers.reserve(256);
    mCacheTextures.reserve(256);
    mCacheRenderTargets.reserve(64);
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

Context::~Context() { mGpuEpochTracker.destroy(); }

Buffer<untyped_tag> pr::Context::make_untyped_buffer(size_t size, size_t stride, bool read_only)
{
    auto const info = buffer_info{size, stride, !read_only, false};
    return {acquireBuffer(info), info};
}

Buffer<untyped_tag> pr::Context::make_untyped_upload_buffer(size_t size, size_t stride, bool read_only)
{
    auto const info = buffer_info{size, stride, !read_only, true};
    auto const handle = acquireBuffer(info);
    return {handle, info, mBackend->getMappedMemory(handle)};
}
