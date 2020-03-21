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
    auto const info = buffer_info{size, stride, allow_uav, true};
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

baked_argument Context::make_argument(const argument& arg, bool usage_compute)
{
    //
    return {{mBackend->createShaderView(arg._srvs, arg._uavs, arg._samplers, usage_compute)}, this};
}


void Context::write_buffer(const buffer& buffer, void const* data, size_t size, size_t offset)
{
    CC_ASSERT(buffer._info.is_mapped && "Attempted to write to non-mapped buffer");
    CC_ASSERT(buffer._info.size_bytes >= size && "Buffer write out of bounds");
    std::memcpy(mBackend->getMappedMemory(buffer._resource.data.handle) + offset, data, size);
}

cached_render_target Context::get_target(tg::isize2 size, phi::format format, unsigned num_samples)
{
    auto const info = render_target_info{format, size.width, size.height, num_samples};
    return {{acquireRenderTarget(info), info}, this};
}

cached_buffer Context::get_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, false};
    return {{acquireBuffer(info), info}, this};
}

cached_buffer Context::get_upload_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, true};
    return {{acquireBuffer(info), info}, this};
}


CompiledFrame Context::compile(Frame& frame)
{
    frame.finalize();

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

void Context::submit(Frame& frame) { submit(compile(frame)); }

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

void Context::present() { mBackend->present(); }

void Context::flush() { mBackend->flushGPU(); }

bool Context::start_capture() { return mBackend->startForcedDiagnosticCapture(); }
bool Context::end_capture() { return mBackend->endForcedDiagnosticCapture(); }

void Context::on_window_resize(tg::isize2 size) { mBackend->onResize(size); }

bool Context::clear_backbuffer_resize() { return mBackend->clearPendingResize(); }

tg::isize2 Context::get_backbuffer_size() const { return mBackend->getBackbufferSize(); }

phi::format Context::get_backbuffer_format() const { return mBackend->getBackbufferFormat(); }

render_target Context::acquire_backbuffer()
{
    auto const backbuffer = mBackend->acquireBackbuffer();
    auto const size = mBackend->getBackbufferSize();
    return {pr::resource{{backbuffer, backbuffer.is_valid() ? acquireGuid() : 0}, nullptr}, // no context pointer, backbuffer is never freed
            render_target_info{mBackend->getBackbufferFormat(), size.width, size.height, 1}};
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

compute_pipeline_state Context::create_compute_pso(phi::arg::shader_arg_shapes arg_shapes, bool has_root_consts, phi::arg::shader_binary shader)
{
    return compute_pipeline_state{{{mBackend->createComputePipelineState(arg_shapes, shader, has_root_consts)}}, this};
}

graphics_pipeline_state Context::create_graphics_pso(phi::arg::vertex_format const& vert_format,
                                                     phi::arg::framebuffer_config const& framebuf_config,
                                                     phi::arg::shader_arg_shapes arg_shapes,
                                                     bool has_root_consts,
                                                     phi::arg::graphics_shaders shader,
                                                     phi::pipeline_config const& config)
{
    return graphics_pipeline_state{{{mBackend->createPipelineState(vert_format, framebuf_config, arg_shapes, has_root_consts, shader, config)}}, this};
}
