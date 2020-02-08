#include "Context.hh"

#include <rich-log/log.hh>

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

image Context::make_image(int width, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t1d, allow_uav, width, 1, 1, num_mips};
    auto const resource = createTexture(info);
    return {resource, info};
}

image Context::make_image(tg::isize2 size, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t2d, allow_uav, size.width, size.height, 1, num_mips};
    auto const resource = createTexture(info);
    return {resource, info};
}

image Context::make_image(tg::isize3 size, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info
        = texture_info{format, phi::texture_dimension::t3d, allow_uav, size.width, size.height, static_cast<unsigned int>(size.depth), num_mips};
    auto const resource = createTexture(info);
    return {resource, info};
}

render_target Context::make_target(tg::isize2 size, phi::format format, unsigned num_samples)
{
    auto const info = render_target_info{format, size.width, size.height, num_samples};
    auto const resource = createRenderTarget(info);
    return {resource, info};
}

buffer Context::make_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, false};
    auto const resource = createBuffer(info);
    return {resource, info};
}

buffer Context::make_upload_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, false};
    auto const resource = createBuffer(info);
    return {resource, info};
}

shader_binary Context::make_shader(cc::string_view code, phi::shader_stage stage)
{
    LOG(warning)("Shader compilation unimplemented");
    return shader_binary{nullptr, 0, stage, 0};
}

baked_argument Context::make_argument(const argument& arg)
{
    //
    return {mBackend->createShaderView(arg._srvs, arg._uavs, arg._samplers, false)};
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

Context::Context(cc::poly_unique_ptr<phi::Backend>&& backend) : mBackend(cc::move(backend)) { initialize(); }

void Context::initialize()
{
    mGpuEpochTracker.initialize(mBackend.get(), 2048);
    mCacheBuffers.reserve(256);
    mCacheTextures.reserve(256);
    mCacheRenderTargets.reserve(64);
}

resource Context::createRenderTarget(const render_target_info& info)
{
    return {mBackend->createRenderTarget(info.format, {info.width, info.height}, info.num_samples), acquireGuid()};
}

resource Context::createTexture(const texture_info& info)
{
    return {mBackend->createTexture(info.format, {info.width, info.height}, info.num_mips, info.dim, info.depth_or_array_size, info.allow_uav), acquireGuid()};
}

resource Context::createBuffer(const buffer_info& info)
{
    pr::resource res;
    if (info.is_mapped)
    {
        res._handle = mBackend->createMappedBuffer(info.size_bytes, info.stride_bytes);
    }
    else
    {
        res._handle = mBackend->createBuffer(info.size_bytes, info.stride_bytes, info.allow_uav);
    }
    res._guid = acquireGuid();
    return res;
}

pr::resource Context::acquireRenderTarget(const render_target_info& info)
{
    auto const lookup = mCacheRenderTargets.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup._handle.is_valid())
    {
        return lookup;
    }
    else
    {
        return createRenderTarget(info);
    }
}

pr::resource Context::acquireTexture(const texture_info& info)
{
    auto const lookup = mCacheTextures.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup._handle.is_valid())
    {
        return lookup;
    }
    else
    {
        return createTexture(info);
    }
}

pr::resource Context::acquireBuffer(const buffer_info& info)
{
    auto const lookup = mCacheBuffers.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup._handle.is_valid())
    {
        return lookup;
    }
    else
    {
        return createBuffer(info);
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

void Context::freeRenderTarget(const render_target_info& info, pr::resource res)
{
    mCacheRenderTargets.free(res, info, mGpuEpochTracker.get_current_epoch_cpu());
}

void Context::freeTexture(const texture_info& info, pr::resource res) { mCacheTextures.free(res, info, mGpuEpochTracker.get_current_epoch_cpu()); }

void Context::freeBuffer(const buffer_info& info, pr::resource res) { mCacheBuffers.free(res, info, mGpuEpochTracker.get_current_epoch_cpu()); }
