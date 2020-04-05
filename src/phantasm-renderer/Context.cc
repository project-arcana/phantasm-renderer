#include "Context.hh"

#include <rich-log/log.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/config.hh>

#include <phantasm-renderer/CompiledFrame.hh>
#include <phantasm-renderer/Frame.hh>
#include <phantasm-renderer/backends.hh>
#include <phantasm-renderer/common/murmur_hash.hh>

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

raii::Frame Context::make_frame(size_t initial_size)
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

shader_binary Context::make_shader(const std::byte* data, size_t size, phi::shader_stage stage)
{
    CC_ASSERT(data != nullptr);

    shader_binary res;
    res.data._stage = stage;
    res.data._data = data;
    res.data._size = size;
    res.data._owning_blob = nullptr;
    murmurhash3_x64_128(res.data._data, static_cast<int>(res.data._size), 0, res.data._hash);

    return res;
}

shader_binary Context::make_shader(cc::string_view code, cc::string_view entrypoint, phi::shader_stage stage)
{
    phi::sc::binary bin;

    auto const sc_target = stage_to_sc_target(stage);
    auto const sc_output = mBackend->getBackendType() == phi::backend_type::d3d12 ? phi::sc::output::dxil : phi::sc::output::spirv;

    {
        auto lg = std::lock_guard(mMutexShaderCompilation);
        bin = mShaderCompiler.compile_binary(code.data(), entrypoint.data(), sc_target, sc_output); // unsynced, mutex: compilation
    }


    shader_binary res;
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
        murmurhash3_x64_128(res.data._data, static_cast<int>(res.data._size), 0, res.data._hash);
    }

    return res;
}

graphics_pipeline_state Context::make_pipeline_state(const graphics_pass_info& gp_wrap, const framebuffer_info& fb)
{
    auto const& gp = gp_wrap._storage.get();
    return graphics_pipeline_state{{{mBackend->createPipelineState({gp.vertex_attributes, gp.vertex_size_bytes}, fb._storage.get(), gp.arg_shapes,
                                                                   gp.has_root_consts, gp_wrap._shaders, gp.graphics_config)}},
                                   this};
}

compute_pipeline_state Context::make_pipeline_state(const compute_pass_info& cp_wrap)
{
    auto const& cp = cp_wrap._storage.get();
    return compute_pipeline_state{{{mBackend->createComputePipelineState(cp.arg_shapes, cp_wrap._shader, cp.has_root_consts)}}, this};
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


CompiledFrame Context::compile(raii::Frame& frame)
{
    frame.finalize();

    if (frame.isEmpty())
    {
        return CompiledFrame(phi::handle::null_command_list, phi::handle::null_event, cc::move(frame.mFreeables));
    }
    else
    {
        auto const event = mGpuEpochTracker.get_event();                                             // intern. synced
        auto const cmdlist = mBackend->recordCommandList(frame.getMemory(), frame.getSize(), event); // intern. synced
        return CompiledFrame(cmdlist, event, cc::move(frame.mFreeables));
    }
}

void Context::submit(raii::Frame& frame) { submit(compile(frame)); }

void Context::submit(CompiledFrame&& frame)
{
    if (frame.cmdlist.is_valid())
    {
        {
            auto const lg = std::lock_guard(mMutexSubmission);
            mBackend->submit(cc::span{frame.cmdlist}); // unsynced, mutex: submission
        }
        mGpuEpochTracker.on_event_submission(frame.event); // intern. synced
    }

    free_all(frame.freeables);
    frame.parent = nullptr;
}

void Context::discard(CompiledFrame&& frame)
{
    if (frame.cmdlist.is_valid())
        mBackend->discard(cc::span{frame.cmdlist});
    if (frame.event.is_valid())
        mBackend->free(cc::span{frame.event});

    free_all(frame.freeables);
    frame.parent = nullptr;
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

Context::Context(phi::window_handle const& window_handle) : mOwnsBackend(true)
{
    phi::backend_config cfg;
#ifndef CC_RELEASE
    cfg.validation = phi::validation_level::on_extended;
#endif
    mBackend = make_vulkan_backend(window_handle, cfg);
    initialize();
}

Context::Context(phi::Backend* backend) : mBackend(backend), mOwnsBackend(false) { initialize(); }

void Context::initialize()
{
    mGpuEpochTracker.initialize(mBackend, 2048);
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

void Context::freeShaderBinary(IDxcBlob* blob)
{
    phi::sc::destroy_blob(blob); // intern. synced
}

void Context::freeShaderView(phi::handle::shader_view sv) { mBackend->free(sv); }

void Context::freePipelineState(phi::handle::pipeline_state ps) { mBackend->free(ps); }

Context::~Context()
{
    // grab all locks (dining philosophers does not apply, nothing else is allowed to contend here)
    auto lg_sub = std::lock_guard(mMutexSubmission);
    auto lg_comp = std::lock_guard(mMutexShaderCompilation);

    // flush GPU
    mBackend->flushGPU();

    // empty all caches
    mCacheGraphicsPSOs.iterate_values([&](phi::handle::pipeline_state pso) { mBackend->free(pso); });
    mCacheComputePSOs.iterate_values([&](phi::handle::pipeline_state pso) { mBackend->free(pso); });
    mCacheGraphicsSVs.iterate_values([&](phi::handle::shader_view sv) { mBackend->free(sv); });
    mCacheComputeSVs.iterate_values([&](phi::handle::shader_view sv) { mBackend->free(sv); });

    mCacheBuffers.clear_all();
    mCacheTextures.clear_all();
    mCacheRenderTargets.clear_all();

    // destroy other components
    mGpuEpochTracker.destroy();
    mShaderCompiler.destroy();

    // if onwing mBackend, destroy and free it
    if (mOwnsBackend)
    {
        mBackend->destroy();
        delete mBackend;
    }
}

uint64_t Context::acquireGuid() { return mResourceGUID.fetch_add(1); }

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

phi::handle::pipeline_state Context::acquire_graphics_pso(murmur_hash hash, graphics_pass_info const& gp, framebuffer_info const& fb)
{
    phi::handle::pipeline_state pso = mCacheGraphicsPSOs.acquire(hash);


    if (!pso.is_valid())
    {
        graphics_pass_info_data const& info = gp._storage.get();
        pso = mBackend->createPipelineState({info.vertex_attributes, info.vertex_size_bytes}, fb._storage.get(), info.arg_shapes,
                                            info.has_root_consts, gp._shaders, info.graphics_config);

        mCacheGraphicsPSOs.insert(pso, hash);
    }
    return pso;
}

phi::handle::pipeline_state Context::acquire_compute_pso(murmur_hash hash, const compute_pass_info& cp)
{
    phi::handle::pipeline_state pso = mCacheComputePSOs.acquire(hash);
    if (!pso.is_valid())
    {
        compute_pass_info_data const& info = cp._storage.get();
        pso = mBackend->createComputePipelineState(info.arg_shapes, cp._shader, info.has_root_consts);
        mCacheComputePSOs.insert(pso, hash);
    }
    return pso;
}

phi::handle::shader_view Context::acquire_graphics_sv(murmur_hash hash, const hashable_storage<shader_view_info>& info_storage)
{
    phi::handle::shader_view sv = mCacheGraphicsSVs.acquire(hash);
    if (!sv.is_valid())
    {
        shader_view_info const& info = info_storage.get();
        sv = mBackend->createShaderView(info.srvs, info.uavs, info.samplers, false);
        mCacheGraphicsSVs.insert(sv, hash);
    }
    return sv;
}

phi::handle::shader_view Context::acquire_compute_sv(murmur_hash hash, const hashable_storage<shader_view_info>& info_storage)
{
    phi::handle::shader_view sv = mCacheComputeSVs.acquire(hash);
    if (!sv.is_valid())
    {
        shader_view_info const& info = info_storage.get();
        sv = mBackend->createShaderView(info.srvs, info.uavs, info.samplers, false);
        mCacheComputeSVs.insert(sv, hash);
    }
    return sv;
}

void Context::free_all(cc::span<const freeable_cached_obj> freeables)
{
    for (auto const& freeable : freeables)
    {
        switch (freeable.type)
        {
        case freeable_cached_obj::graphics_pso:
            free_graphics_pso(freeable.hash);
            break;
        case freeable_cached_obj::compute_pso:
            free_compute_pso(freeable.hash);
            break;
        case freeable_cached_obj::graphics_sv:
            free_graphics_sv(freeable.hash);
            break;
        case freeable_cached_obj::compute_sv:
            free_compute_sv(freeable.hash);
            break;
        }
    }
}

void Context::free_graphics_pso(murmur_hash hash) { mCacheGraphicsPSOs.free(hash, mGpuEpochTracker.get_current_epoch_cpu()); }
void Context::free_compute_pso(murmur_hash hash) { mCacheComputePSOs.free(hash, mGpuEpochTracker.get_current_epoch_cpu()); }

void Context::free_graphics_sv(murmur_hash hash) { mCacheGraphicsSVs.free(hash, mGpuEpochTracker.get_current_epoch_cpu()); }
void Context::free_compute_sv(murmur_hash hash) { mCacheComputeSVs.free(hash, mGpuEpochTracker.get_current_epoch_cpu()); }
