#include "Context.hh"

#include <rich-log/log.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/config.hh>
#include <phantasm-hardware-interface/detail/byte_util.hh>
#include <phantasm-hardware-interface/detail/format_size.hh>

#include <phantasm-renderer/CompiledFrame.hh>
#include <phantasm-renderer/Frame.hh>
#include <phantasm-renderer/common/murmur_hash.hh>
#include <phantasm-renderer/detail/backends.hh>

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

auto_texture Context::make_texture(int width, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t1d, allow_uav, width, 1, 1, num_mips};
    return {createTexture(info), this};
}

auto_texture Context::make_texture(tg::isize2 size, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t2d, allow_uav, size.width, size.height, 1, num_mips};
    return {createTexture(info), this};
}

auto_texture Context::make_texture(tg::isize3 size, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info
        = texture_info{format, phi::texture_dimension::t3d, allow_uav, size.width, size.height, static_cast<unsigned int>(size.depth), num_mips};
    return {createTexture(info), this};
}

auto_texture Context::make_texture_cube(tg::isize2 size, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t2d, allow_uav, size.width, size.height, 6, num_mips};
    return {createTexture(info), this};
}

auto_texture Context::make_texture_array(int width, unsigned num_elems, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t1d, allow_uav, width, 1, num_elems, num_mips};
    return {createTexture(info), this};
}

auto_texture Context::make_texture_array(tg::isize2 size, unsigned num_elems, phi::format format, unsigned num_mips, bool allow_uav)
{
    auto const info = texture_info{format, phi::texture_dimension::t2d, allow_uav, size.width, size.height, num_elems, num_mips};
    return {createTexture(info), this};
}

auto_render_target Context::make_target(tg::isize2 size, phi::format format, unsigned num_samples)
{
    auto const info = render_target_info{format, size.width, size.height, num_samples};
    return {createRenderTarget(info), this};
}

auto_buffer Context::make_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, false};
    return {createBuffer(info), this};
}

auto_buffer Context::make_upload_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, true};
    return {createBuffer(info), this};
}

auto_shader_binary Context::make_shader(std::byte const* data, size_t size, phi::shader_stage stage)
{
    CC_ASSERT(data != nullptr);

    shader_binary res;
    res._stage = stage;
    res._data = data;
    res._size = size;
    res._owning_blob = nullptr;
    murmurhash3_x64_128(res._data, static_cast<int>(res._size), 0, res._hash);

    return {res, this};
}

auto_shader_binary Context::make_shader(cc::string_view code, cc::string_view entrypoint, phi::shader_stage stage)
{
    phi::sc::binary bin;

    auto const sc_target = stage_to_sc_target(stage);
    auto const sc_output = mBackend->getBackendType() == phi::backend_type::d3d12 ? phi::sc::output::dxil : phi::sc::output::spirv;

    {
        auto lg = std::lock_guard(mMutexShaderCompilation); // unsynced, mutex: compilation
        bin = mShaderCompiler.compile_binary(code.data(), entrypoint.data(), sc_target, sc_output);
    }

    if (bin.data == nullptr)
    {
        LOG(warning)("Failed to compile shader");
        return {};
    }
    else
    {
        auto res = make_shader(bin.data, bin.size, stage);
        res.data._owning_blob = bin.internal_blob;
        return res;
    }
}

auto_prebuilt_argument Context::make_graphics_argument(const argument& arg)
{
    auto const& info = arg._info.get();
    return {prebuilt_argument{mBackend->createShaderView(info.srvs, info.uavs, info.samplers, false), phi::handle::null_resource, 0}, this};
}

auto_prebuilt_argument Context::make_compute_argument(const argument& arg)
{
    auto const& info = arg._info.get();
    return {prebuilt_argument{mBackend->createShaderView(info.srvs, info.uavs, info.samplers, true), phi::handle::null_resource, 0}, this};
}

auto_graphics_pipeline_state Context::make_pipeline_state(const graphics_pass_info& gp_wrap, const framebuffer_info& fb)
{
    auto const& gp = gp_wrap._storage.get();
    return auto_graphics_pipeline_state{{{mBackend->createPipelineState({gp.vertex_attributes, gp.vertex_size_bytes}, fb._storage.get(),
                                                                        gp.arg_shapes, gp.has_root_consts, gp_wrap._shaders, gp.graphics_config)}},
                                        this};
}

auto_compute_pipeline_state Context::make_pipeline_state(const compute_pass_info& cp_wrap)
{
    auto const& cp = cp_wrap._storage.get();
    return auto_compute_pipeline_state{{{mBackend->createComputePipelineState(cp.arg_shapes, cp_wrap._shader, cp.has_root_consts)}}, this};
}

void Context::free(const raw_resource& resource) { mBackend->free(resource.handle); }

void Context::free_to_cache(const buffer& buffer) { freeCachedBuffer(buffer.info, buffer.res); }

void Context::free_to_cache(const texture& texture) { freeCachedTexture(texture.info, texture.res); }

void Context::free_to_cache(const render_target& rt) { freeCachedTarget(rt.info, rt.res); }


void Context::write_buffer(const buffer& buffer, void const* data, size_t size, size_t offset)
{
    CC_ASSERT(buffer.info.is_mapped && "Attempted to write to non-mapped buffer");
    CC_ASSERT(buffer.info.size_bytes >= size && "Buffer write out of bounds");
    std::memcpy(mBackend->getMappedMemory(buffer.res.handle) + offset, data, size);
}

void Context::flush_buffer_writes(const buffer& buffer)
{
    CC_ASSERT(buffer.info.is_mapped && "Attempted to flush writes to non-mapped buffer");
    mBackend->flushMappedMemory(buffer.res.handle);
}

std::byte* Context::get_buffer_map(const buffer& buffer)
{
    CC_ASSERT(buffer.info.is_mapped && "Attempted to get map from non-mapped buffer");
    return mBackend->getMappedMemory(buffer.res.handle);
}

cached_render_target Context::get_target(tg::isize2 size, phi::format format, unsigned num_samples)
{
    auto const info = render_target_info{format, size.width, size.height, num_samples};
    return {acquireRenderTarget(info), this};
}

cached_buffer Context::get_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, false};
    return {acquireBuffer(info), this};
}

cached_buffer Context::get_upload_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, true};
    return {acquireBuffer(info), this};
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

gpu_epoch_t Context::submit(raii::Frame& frame) { return submit(compile(frame)); }

gpu_epoch_t Context::submit(CompiledFrame&& frame)
{
    gpu_epoch_t res = 0;

    if (frame.cmdlist.is_valid())
    {
        {
            auto const lg = std::lock_guard(mMutexSubmission);
            mBackend->submit(cc::span{frame.cmdlist}); // unsynced, mutex: submission
        }
        res = mGpuEpochTracker.on_event_submission(frame.event); // intern. synced
    }

    free_all(frame.freeables);
    frame.parent = nullptr;
    return res;
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

bool Context::flush(gpu_epoch_t epoch)
{
    if (mGpuEpochTracker.get_current_epoch_gpu() >= epoch)
        return false;

    flush();
    return true;
}

bool Context::start_capture() { return mBackend->startForcedDiagnosticCapture(); }
bool Context::end_capture() { return mBackend->endForcedDiagnosticCapture(); }

void Context::on_window_resize(tg::isize2 size) { mBackend->onResize(size); }

bool Context::clear_backbuffer_resize() { return mBackend->clearPendingResize(); }

tg::isize2 Context::get_backbuffer_size() const { return mBackend->getBackbufferSize(); }

phi::format Context::get_backbuffer_format() const { return mBackend->getBackbufferFormat(); }

unsigned Context::get_num_backbuffers() const { return mBackend->getNumBackbuffers(); }

unsigned Context::calculate_texture_upload_size(tg::isize3 size, phi::format fmt, unsigned num_mips) const
{
    // calculate number of mips if zero is given
    num_mips = num_mips > 0 ? num_mips : calculate_num_mip_levels({size.width, size.height});
    auto const bytes_per_pixel = phi::detail::format_size_bytes(fmt);
    auto res_bytes = 0u;

    for (auto a = 0; a < size.depth; ++a)
    {
        for (auto mip = 0u; mip < num_mips; ++mip)
        {
            auto const mip_width = cc::max(unsigned(tg::floor(size.width / tg::pow(2.f, float(mip)))), 1u);
            auto const mip_height = cc::max(unsigned(tg::floor(size.height / tg::pow(2.f, float(mip)))), 1u);

            auto const row_pitch = phi::mem::align_up(bytes_per_pixel * mip_width, 256);
            auto const custom_offset = row_pitch * mip_height;
            res_bytes += custom_offset;
        }
    }

    return res_bytes;
}

render_target Context::acquire_backbuffer()
{
    auto const backbuffer = mBackend->acquireBackbuffer();
    auto const size = mBackend->getBackbufferSize();
    return {{backbuffer, backbuffer.is_valid() ? acquireGuid() : 0}, {mBackend->getBackbufferFormat(), size.width, size.height, 1}};
}

Context::Context(phi::window_handle const& window_handle, backend_type type) { initialize(window_handle, type); }

Context::Context(phi::Backend* backend) { initialize(backend); }

void Context::initialize(const phi::window_handle& window_handle, backend_type type)
{
    CC_RUNTIME_ASSERT(mBackend == nullptr && "pr::Context double initialize");

    mOwnsBackend = true;
    phi::backend_config cfg;
#ifndef CC_RELEASE
    cfg.validation = phi::validation_level::on_extended;
#endif

    mBackend = detail::make_backend(type, window_handle, cfg);
    CC_RUNTIME_ASSERT(mBackend != nullptr && "Failed to create backend");
    internalInitialize();
}

void Context::initialize(phi::Backend* backend)
{
    CC_RUNTIME_ASSERT(mBackend == nullptr && "pr::Context double initialize");

    mBackend = backend;
    mOwnsBackend = false;
    CC_RUNTIME_ASSERT(mBackend != nullptr && "Invalid backend received");
    internalInitialize();
}

void Context::destroy()
{
    if (mBackend != nullptr)
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

        mBackend = nullptr;
    }
}

void Context::internalInitialize()
{
    mGpuEpochTracker.initialize(mBackend, 2048);
    mCacheBuffers.reserve(256);
    mCacheTextures.reserve(256);
    mCacheRenderTargets.reserve(64);
    mShaderCompiler.initialize();
}

render_target Context::createRenderTarget(const render_target_info& info)
{
    return {{mBackend->createRenderTarget(info.format, {info.width, info.height}, info.num_samples), acquireGuid()}, info};
}

texture Context::createTexture(const texture_info& info)
{
    return {{mBackend->createTexture(info.fmt, {info.width, info.height}, info.num_mips, info.dim, info.depth_or_array_size, info.allow_uav), acquireGuid()}, info};
}

buffer Context::createBuffer(const buffer_info& info)
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
    return {{handle, acquireGuid()}, info};
}

render_target Context::acquireRenderTarget(const render_target_info& info)
{
    auto lookup = mCacheRenderTargets.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.handle.is_valid())
    {
        return {lookup, info};
    }
    else
    {
        return createRenderTarget(info);
    }
}

texture Context::acquireTexture(const texture_info& info)
{
    auto lookup = mCacheTextures.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.handle.is_valid())
    {
        return {lookup, info};
    }
    else
    {
        return createTexture(info);
    }
}

buffer Context::acquireBuffer(const buffer_info& info)
{
    auto lookup = mCacheBuffers.acquire(info, mGpuEpochTracker.get_current_epoch_gpu());
    if (lookup.handle.is_valid())
    {
        return {lookup, info};
    }
    else
    {
        return createBuffer(info);
    }
}

void Context::freeShaderBinary(IDxcBlob* blob)
{
    phi::sc::destroy_blob(blob); // intern. synced
}

void Context::freeShaderView(phi::handle::shader_view sv) { mBackend->free(sv); }

void Context::freePipelineState(phi::handle::pipeline_state ps) { mBackend->free(ps); }

uint64_t Context::acquireGuid() { return mResourceGUID.fetch_add(1); }

void Context::freeCachedTarget(const render_target_info& info, raw_resource res)
{
    mCacheRenderTargets.free(res, info, mGpuEpochTracker.get_current_epoch_cpu());
}

void Context::freeCachedTexture(const texture_info& info, raw_resource res)
{
    mCacheTextures.free(res, info, mGpuEpochTracker.get_current_epoch_cpu());
}

void Context::freeCachedBuffer(const buffer_info& info, raw_resource res) { mCacheBuffers.free(res, info, mGpuEpochTracker.get_current_epoch_cpu()); }

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
