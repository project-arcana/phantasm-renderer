#include "Context.hh"

#include <rich-log/log.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/config.hh>

#include <phantasm-renderer/CompiledFrame.hh>
#include <phantasm-renderer/Frame.hh>
#include <phantasm-renderer/backends.hh>

using namespace pr;

namespace
{
phi::sc::target stage_to_sc_target(phi::shader_stage stage)
{
    switch (stage)
    {
    case phi::shader_stage::vertex:
        return phi::sc::target::vertex;
    case phi::shader_stage::hull:
        return phi::sc::target::hull;
    case phi::shader_stage::domain:
        return phi::sc::target::domain;
    case phi::shader_stage::geometry:
        return phi::sc::target::geometry;
    case phi::shader_stage::pixel:
        return phi::sc::target::pixel;
    case phi::shader_stage::compute:
        return phi::sc::target::compute;
    default:
        LOG(warning)("Unsupported shader stage for online compilation");
        return phi::sc::target::pixel;
    }
}

}

Frame Context::make_frame(size_t initial_size)
{
    // TODO: pool the memory blocks for frames
    return {this, initial_size};
}

image Context::make_image(int width, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t1d, allow_uav, width, 1, 1, num_mips};
    return {createTexture(info), info};
}

image Context::make_image(tg::isize2 size, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t2d, allow_uav, size.width, size.height, 1, num_mips};
    return {createTexture(info), info};
}

image Context::make_image(tg::isize3 size, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info
        = texture_info{format, phi::texture_dimension::t3d, allow_uav, size.width, size.height, static_cast<unsigned int>(size.depth), num_mips};
    return {createTexture(info), info};
}

render_target Context::make_target(tg::isize2 size, phi::format format, unsigned num_samples)
{
    auto const info = render_target_info{format, size.width, size.height, num_samples};
    return {createRenderTarget(info), info};
}

buffer Context::make_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, false};
    return {createBuffer(info), info};
}

buffer Context::make_upload_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, false};
    return {createBuffer(info), info};
}

shader_binary Context::make_shader(cc::string_view code, cc::string_view entrypoint, phi::shader_stage stage)
{
    auto const bin = mShaderCompiler.compile_binary(code.data(), entrypoint.data(), stage_to_sc_target(stage),
                                                    mBackend->getBackendType() == phi::backend_type::d3d12 ? phi::sc::output::dxil : phi::sc::output::spirv);

    shader_binary res;
    res.data._parent = this;
    res.data._stage = stage;

    if (bin.data == nullptr)
    {
        LOG(warning)("Failed to compile shader");
    }
    else
    {
        res.data._data = bin.data;
        res.data._size = bin.size;
        res.data._owning_blob = bin.internal_blob;
        res.data._guid = acquireGuid();
    }

    return res;
}

baked_shader_view Context::make_argument(const shader_view& arg, bool usage_compute)
{
    //
    return {{mBackend->createShaderView(arg._srvs, arg._uavs, arg._samplers, usage_compute)}, this};
}

graphics_pipeline_state Context::make_graphics_pipeline_state(const phi::arg::vertex_format& vert_format,
                                                              phi::arg::framebuffer_config const& framebuf_config,
                                                              phi::arg::shader_arg_shapes arg_shapes,
                                                              bool has_root_consts,
                                                              phi::arg::graphics_shaders shader,
                                                              phi::pipeline_config const& config)
{
    return {{{mBackend->createPipelineState(vert_format, framebuf_config, arg_shapes, has_root_consts, shader, config)}}, this};
}

compute_pipeline_state Context::make_compute_pipeline_state(phi::arg::shader_arg_shapes arg_shapes, bool has_root_constants, const shader_binary& shader)
{
    auto const res_handle = mBackend->createComputePipelineState(arg_shapes, {shader.data._data, shader.data._size}, has_root_constants);
    return {{{res_handle}}, this};
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
    mShaderCompiler.initialize();
}

resource Context::createRenderTarget(const render_target_info& info)
{
    return {{mBackend->createRenderTarget(info.format, {info.width, info.height}, info.num_samples), acquireGuid()}, this};
}

resource Context::createTexture(const texture_info& info)
{
    return {{mBackend->createTexture(info.format, {info.width, info.height}, info.num_mips, info.dim, info.depth_or_array_size, info.allow_uav), acquireGuid()}, this};
}

resource Context::createBuffer(const buffer_info& info)
{
    phi::handle::resource handle;
    if (info.is_mapped)
    {
        handle = mBackend->createMappedBuffer(info.size_bytes, info.stride_bytes);
    }
    else
    {
        handle = mBackend->createBuffer(info.size_bytes, info.stride_bytes, info.allow_uav);
    }
    return {{handle, acquireGuid()}, this};
}

pr::resource Context::acquireRenderTarget(const render_target_info& info)
{
    auto lookup = mCacheRenderTargets.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.data.handle.is_valid())
    {
        return cc::move(lookup);
    }
    else
    {
        return createRenderTarget(info);
    }
}

pr::resource Context::acquireTexture(const texture_info& info)
{
    auto lookup = mCacheTextures.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.data.handle.is_valid())
    {
        return cc::move(lookup);
    }
    else
    {
        return createTexture(info);
    }
}

pr::resource Context::acquireBuffer(const buffer_info& info)
{
    auto lookup = mCacheBuffers.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.data.handle.is_valid())
    {
        return cc::move(lookup);
    }
    else
    {
        return createBuffer(info);
    }
}

void Context::freeResource(phi::handle::resource res) { mBackend->free(res); }

void Context::freeShaderBinary(IDxcBlob* blob) { phi::sc::destroy_blob(blob); }

void Context::freeShaderView(phi::handle::shader_view sv) { mBackend->free(sv); }

void Context::freePipelineState(phi::handle::pipeline_state ps) { mBackend->free(ps); }

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
    mShaderCompiler.destroy();
}

void Context::freeCachedTarget(const render_target_info& info, pr::resource&& res)
{
    mCacheRenderTargets.free(cc::move(res), info, mGpuEpochTracker.get_current_epoch_cpu());
}

void Context::freeCachedTexture(const texture_info& info, pr::resource&& res)
{
    mCacheTextures.free(cc::move(res), info, mGpuEpochTracker.get_current_epoch_cpu());
}

void Context::freeCachedBuffer(const buffer_info& info, resource&& res)
{
    mCacheBuffers.free(cc::move(res), info, mGpuEpochTracker.get_current_epoch_cpu());
}
