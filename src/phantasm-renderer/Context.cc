#include "Context.hh"

#include <typed-geometry/tg.hh>

#include <clean-core/xxHash.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/config.hh>
#include <phantasm-hardware-interface/detail/byte_util.hh>
#include <phantasm-hardware-interface/detail/format_size.hh>
#include <phantasm-hardware-interface/util.hh>

#include <phantasm-renderer/CompiledFrame.hh>
#include <phantasm-renderer/Frame.hh>
#include <phantasm-renderer/common/log.hh>
#include <phantasm-renderer/detail/backends.hh>

using namespace pr;

namespace
{
dxcw::target stage_to_dxcw_target(phi::shader_stage stage)
{
    switch (stage)
    {
    case phi::shader_stage::vertex:
        return dxcw::target::vertex;
    case phi::shader_stage::hull:
        return dxcw::target::hull;
    case phi::shader_stage::domain:
        return dxcw::target::domain;
    case phi::shader_stage::geometry:
        return dxcw::target::geometry;
    case phi::shader_stage::pixel:
        return dxcw::target::pixel;
    case phi::shader_stage::compute:
        return dxcw::target::compute;
    default:
        PR_LOG_WARN("Unsupported shader stage for online compilation");
        return dxcw::target::pixel;
    }
}

}

raii::Frame Context::make_frame(size_t initial_size, cc::allocator* alloc) { return pr::raii::Frame{this, initial_size, alloc}; }

auto_texture Context::make_texture(int width, phi::format format, unsigned num_mips, bool allow_uav, char const* debug_name)
{
    auto const info = texture_info{format, phi::texture_dimension::t1d, allow_uav, width, 1, 1, num_mips};
    return {createTexture(info, debug_name), this};
}

auto_texture Context::make_texture(tg::isize2 size, phi::format format, unsigned num_mips, bool allow_uav, char const* debug_name)
{
    auto const info = texture_info{format, phi::texture_dimension::t2d, allow_uav, size.width, size.height, 1, num_mips};
    return {createTexture(info, debug_name), this};
}

auto_texture Context::make_texture(tg::isize3 size, phi::format format, unsigned num_mips, bool allow_uav, char const* debug_name)
{
    auto const info
        = texture_info{format, phi::texture_dimension::t3d, allow_uav, size.width, size.height, static_cast<unsigned int>(size.depth), num_mips};
    return {createTexture(info, debug_name), this};
}

auto_texture Context::make_texture_cube(tg::isize2 size, phi::format format, unsigned num_mips, bool allow_uav, char const* debug_name)
{
    auto const info = texture_info{format, phi::texture_dimension::t2d, allow_uav, size.width, size.height, 6, num_mips};
    return {createTexture(info, debug_name), this};
}

auto_texture Context::make_texture_array(int width, unsigned num_elems, phi::format format, unsigned num_mips, bool allow_uav, char const* debug_name)
{
    auto const info = texture_info{format, phi::texture_dimension::t1d, allow_uav, width, 1, num_elems, num_mips};
    return {createTexture(info, debug_name), this};
}

auto_texture Context::make_texture_array(tg::isize2 size, unsigned num_elems, phi::format format, unsigned num_mips, bool allow_uav, char const* debug_name)
{
    auto const info = texture_info{format, phi::texture_dimension::t2d, allow_uav, size.width, size.height, num_elems, num_mips};
    return {createTexture(info, debug_name), this};
}

auto_texture Context::make_texture(const texture_info& info, char const* debug_name) { return {createTexture(info, debug_name), this}; }

auto_render_target Context::make_target(tg::isize2 size, phi::format format, unsigned num_samples, unsigned array_size, char const* debug_name)
{
    auto const info = render_target_info::create(format, size, num_samples, array_size);
    return {createRenderTarget(info, debug_name), this};
}

auto_render_target Context::make_target(tg::isize2 size, phi::format format, unsigned num_samples, unsigned array_size, phi::rt_clear_value optimized_clear, char const* debug_name)
{
    auto const info = render_target_info::create(format, size, num_samples, array_size, optimized_clear);
    return {createRenderTarget(info, debug_name), this};
}

auto_render_target Context::make_target(const render_target_info& info, char const* debug_name)
{
    return {createRenderTarget(info, debug_name), this};
}

auto_buffer Context::make_buffer(unsigned size, unsigned stride, bool allow_uav, char const* debug_name)
{
    auto const info = buffer_info{size, stride, allow_uav, phi::resource_heap::gpu};
    return {createBuffer(info, debug_name), this};
}

auto_buffer Context::make_upload_buffer(unsigned size, unsigned stride, char const* debug_name)
{
    auto const info = buffer_info{size, stride, false, phi::resource_heap::upload};
    return {createBuffer(info, debug_name), this};
}

auto_buffer Context::make_readback_buffer(unsigned size, unsigned stride, char const* debug_name)
{
    auto const info = buffer_info{size, stride, false, phi::resource_heap::readback};
    return {createBuffer(info, debug_name), this};
}

auto_buffer Context::make_buffer(const buffer_info& info, char const* debug_name) { return {createBuffer(info, debug_name), this}; }

auto_shader_binary Context::make_shader(cc::span<cc::byte const> data, phi::shader_stage stage)
{
    CC_ASSERT(data.data() != nullptr);

    shader_binary res;
    res._stage = stage;
    res._data = data.data();
    res._size = data.size();
    res._owning_blob = nullptr;
    res._hash = cc::hash_xxh3({res._data, res._size}, 0);

    return {res, this};
}

auto_shader_binary Context::make_shader(cc::string_view code, cc::string_view entrypoint, phi::shader_stage stage)
{
    dxcw::binary bin;

    auto const sc_target = stage_to_dxcw_target(stage);
    auto const sc_output = mBackend->getBackendType() == phi::backend_type::d3d12 ? dxcw::output::dxil : dxcw::output::spirv;

    {
        auto lg = std::lock_guard(mMutexShaderCompilation); // unsynced, mutex: compilation
        bin = mShaderCompiler.compile_binary(code.data(), entrypoint.data(), sc_target, sc_output);
    }

    if (bin.data == nullptr)
    {
        PR_LOG_WARN("Failed to compile shader");
        return {};
    }
    else
    {
        auto res = make_shader({bin.data, bin.size}, stage);
        res.data._owning_blob = bin.internal_blob;
        return res;
    }
}

auto_prebuilt_argument Context::make_graphics_argument(const argument& arg)
{
    auto const& info = arg._info.get();
    return {prebuilt_argument{mBackend->createShaderView(info.srvs, info.uavs, info.samplers, false)}, this};
}

auto_prebuilt_argument Context::make_graphics_argument(cc::span<const phi::resource_view> srvs,
                                                       cc::span<const phi::resource_view> uavs,
                                                       cc::span<const phi::sampler_config> samplers)
{
    return {prebuilt_argument{mBackend->createShaderView(srvs, uavs, samplers, false)}, this};
}

auto_prebuilt_argument Context::make_compute_argument(const argument& arg)
{
    auto const& info = arg._info.get();
    return {prebuilt_argument{mBackend->createShaderView(info.srvs, info.uavs, info.samplers, true)}, this};
}

auto_prebuilt_argument Context::make_compute_argument(cc::span<const phi::resource_view> srvs,
                                                      cc::span<const phi::resource_view> uavs,
                                                      cc::span<const phi::sampler_config> samplers)
{
    return {prebuilt_argument{mBackend->createShaderView(srvs, uavs, samplers, true)}, this};
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

auto_fence Context::make_fence() { return auto_fence{{mBackend->createFence()}, this}; }

auto_query_range Context::make_query_range(phi::query_type type, unsigned num_queries)
{
    auto const handle = mBackend->createQueryRange(type, num_queries);
    return auto_query_range{{handle, type, num_queries}, this};
}

auto_swapchain Context::make_swapchain(const phi::window_handle& window_handle, tg::isize2 initial_size, pr::present_mode mode, unsigned num_backbuffers)
{
    return {{mBackend->createSwapchain(window_handle, initial_size, mode, num_backbuffers)}, this};
}

void Context::free_untyped(phi::handle::resource resource) { mBackend->free(resource); }

void Context::free(const fence& f) { mBackend->free(cc::span{f.handle}); }
void Context::free(const query_range& q) { mBackend->free(q.handle); }
void Context::free(swapchain const& sc) { mBackend->free(sc.handle); }

void Context::free_deferred(buffer const& buf) { free_deferred(buf.res.handle); }
void Context::free_deferred(texture const& tex) { free_deferred(tex.res.handle); }
void Context::free_deferred(render_target const& rt) { free_deferred(rt.res.handle); }
void Context::free_deferred(raw_resource const& res) { free_deferred(res.handle); }
void Context::free_deferred(phi::handle::resource res) { mDeferredQueue.free(*this, res); }

void Context::free_to_cache_untyped(const raw_resource& resource, const generic_resource_info& info)
{
    switch (info.type)
    {
    case phi::arg::create_resource_info::e_resource_render_target:
        return freeCachedTarget(info.info_render_target, resource);
    case phi::arg::create_resource_info::e_resource_texture:
        return freeCachedTexture(info.info_texture, resource);
    case phi::arg::create_resource_info::e_resource_buffer:
        return freeCachedBuffer(info.info_buffer, resource);
    default:
        CC_ASSERT(false && "invalid type");
    }
    CC_UNREACHABLE("invalid type");
}

void Context::free_to_cache(const buffer& buffer) { freeCachedBuffer(buffer.info, buffer.res); }

void Context::free_to_cache(const texture& texture) { freeCachedTexture(texture.info, texture.res); }

void Context::free_to_cache(const render_target& rt) { freeCachedTarget(rt.info, rt.res); }

void Context::write_to_buffer_raw(const buffer& buffer, cc::span<std::byte const> data, size_t offset_in_buffer)
{
    CC_ASSERT(buffer.info.heap == phi::resource_heap::upload && "Attempted to write to non-upload buffer");
    CC_ASSERT(buffer.info.size_bytes >= data.size_bytes() + offset_in_buffer && "Buffer write out of bounds");

    int const map_begin = int(offset_in_buffer);
    int const map_end = int(offset_in_buffer + data.size_bytes());

    std::byte* const map = map_buffer(buffer, map_begin, map_end);
    std::memcpy(map, data.data(), data.size_bytes());
    unmap_buffer(buffer, map_begin, map_end);
}

void Context::read_from_buffer_raw(const buffer& buffer, cc::span<std::byte> out_data, size_t offset_in_buffer)
{
    CC_ASSERT(buffer.info.heap == phi::resource_heap::readback && "Attempted to read from non-readback buffer");
    CC_ASSERT(buffer.info.size_bytes >= out_data.size_bytes() + offset_in_buffer && "Buffer read out of bounds");

    int const map_begin = int(offset_in_buffer);
    int const map_end = int(offset_in_buffer + out_data.size_bytes());

    std::byte const* const map = map_buffer(buffer, map_begin, map_end);
    std::memcpy(out_data.data(), map, out_data.size_bytes());
    unmap_buffer(buffer, map_begin, map_end);
}

void Context::signal_fence_cpu(const fence& fence, uint64_t new_value) { mBackend->signalFenceCPU(fence.handle, new_value); }

void Context::wait_fence_cpu(const fence& fence, uint64_t wait_value) { mBackend->waitFenceCPU(fence.handle, wait_value); }

uint64_t Context::get_fence_value(const fence& fence) { return mBackend->getFenceValue(fence.handle); }

std::byte* Context::map_buffer(const buffer& buffer, int begin, int end) { return mBackend->mapBuffer(buffer.res.handle, begin, end); }

void Context::unmap_buffer(const buffer& buffer, int begin, int end) { mBackend->unmapBuffer(buffer.res.handle, begin, end); }

cached_render_target Context::get_target(tg::isize2 size, phi::format format, unsigned num_samples, unsigned array_size)
{
    auto const info = render_target_info::create(format, size, num_samples, array_size);
    return {acquireRenderTarget(info), this};
}

cached_render_target Context::get_target(tg::isize2 size, format format, unsigned num_samples, unsigned array_size, phi::rt_clear_value optimized_clear)
{
    auto const info = render_target_info::create(format, size, num_samples, array_size, optimized_clear);
    return {acquireRenderTarget(info), this};
}

cached_render_target Context::get_target(const render_target_info& info) { return {acquireRenderTarget(info), this}; }

cached_buffer Context::get_buffer(unsigned size, unsigned stride, bool allow_uav)
{
    auto const info = buffer_info{size, stride, allow_uav, phi::resource_heap::gpu};
    return {acquireBuffer(info), this};
}

cached_buffer Context::get_upload_buffer(unsigned size, unsigned stride)
{
    auto const info = buffer_info{size, stride, false, phi::resource_heap::upload};
    return {acquireBuffer(info), this};
}

cached_buffer Context::get_readback_buffer(unsigned size, unsigned stride)
{
    auto const info = buffer_info{size, stride, false, phi::resource_heap::readback};
    return {acquireBuffer(info), this};
}

cached_buffer Context::get_buffer(const buffer_info& info) { return {acquireBuffer(info), this}; }

cached_texture Context::get_texture(const texture_info& info) { return {acquireTexture(info), this}; }

raw_resource Context::make_untyped_unlocked(const generic_resource_info& info, const char* debug_name)
{
    switch (info.type)
    {
    case phi::arg::create_resource_info::e_resource_render_target:
        return createRenderTarget(info.info_render_target, debug_name).res;
    case phi::arg::create_resource_info::e_resource_texture:
        return createTexture(info.info_texture, debug_name).res;
    case phi::arg::create_resource_info::e_resource_buffer:
        return createBuffer(info.info_buffer, debug_name).res;
    default:
        CC_ASSERT(false && "invalid type");
        return {};
    }
    CC_UNREACHABLE("invalid type");
}

raw_resource Context::get_untyped_unlocked(const generic_resource_info& info)
{
    switch (info.type)
    {
    case phi::arg::create_resource_info::e_resource_render_target:
        return acquireRenderTarget(info.info_render_target).res;
    case phi::arg::create_resource_info::e_resource_texture:
        return acquireTexture(info.info_texture).res;
    case phi::arg::create_resource_info::e_resource_buffer:
        return acquireBuffer(info.info_buffer).res;
    default:
        CC_ASSERT(false && "invalid type");
        return {};
    }
    CC_UNREACHABLE("invalid type");
}


CompiledFrame Context::compile(raii::Frame&& frame)
{
    frame.finalize();

    if (frame.is_empty())
    {
        return CompiledFrame(this, phi::handle::null_command_list, cc::move(frame.mFreeables), cc::move(frame.mDeferredFreeResources), phi::handle::null_swapchain);
    }
    else
    {
        auto const cmdlist = mBackend->recordCommandList(frame.getMemory(), frame.getSize()); // intern. synced
        return CompiledFrame(this, cmdlist, cc::move(frame.mFreeables), cc::move(frame.mDeferredFreeResources), frame.mPresentAfterSubmitRequest);
    }
}

gpu_epoch_t Context::submit(raii::Frame&& frame) { return submit(compile(cc::move(frame))); }

gpu_epoch_t Context::submit(CompiledFrame&& frame)
{
    gpu_epoch_t res = 0;

    if (frame.cmdlist.is_valid())
    {
        mGpuEpochTracker._cached_epoch_gpu = mGpuEpochTracker.get_current_epoch_gpu(mBackend);

        // phi::fence_operation wait_op = {mGpuEpochTracker._fence, mGpuEpochTracker._current_epoch_cpu - 1};
        phi::fence_operation signal_op = {mGpuEpochTracker._fence, mGpuEpochTracker._current_epoch_cpu};

        {
            // unsynced, mutex: submission
            auto const lg = std::lock_guard(mMutexSubmission);
            mBackend->submit(cc::span{frame.cmdlist}, phi::queue_type::direct, {}, cc::span{signal_op});
        }

        // increment CPU epoch after signalling
        ++mGpuEpochTracker._current_epoch_cpu;

        res = mGpuEpochTracker._current_epoch_cpu;

        if (frame.present_after_submit_swapchain.is_valid())
        {
            present({frame.present_after_submit_swapchain});
        }
    }

    free_all(frame.freeables);

    if (!frame.deferred_free_resources.empty())
        mDeferredQueue.free_range(*this, frame.deferred_free_resources);

    frame.parent = nullptr;
    return res;
}

void Context::discard(CompiledFrame&& frame)
{
    if (frame.cmdlist.is_valid())
        mBackend->discard(cc::span{frame.cmdlist});
    free_all(frame.freeables);

    if (!frame.deferred_free_resources.empty())
        mDeferredQueue.free_range(*this, frame.deferred_free_resources);

    frame.parent = nullptr;
}

void Context::present(swapchain const& sc)
{
#ifdef CC_ENABLE_ASSERTIONS
    CC_ASSERT(mSafetyState.did_acquire_before_present && "Context::present without prior acquire_backbuffer");
#endif
    mBackend->present(sc.handle);
#ifdef CC_ENABLE_ASSERTIONS
    mSafetyState.did_acquire_before_present = false;
#endif
}

void Context::flush() { mBackend->flushGPU(); }

bool Context::flush(gpu_epoch_t epoch)
{
    if (mGpuEpochTracker._cached_epoch_gpu >= epoch)
        return false;

    flush();
    return true;
}

bool Context::start_capture() { return mBackend->startForcedDiagnosticCapture(); }
bool Context::stop_capture() { return mBackend->endForcedDiagnosticCapture(); }

void Context::on_window_resize(swapchain const& sc, tg::isize2 size) { mBackend->onResize(sc.handle, size); }

bool Context::clear_backbuffer_resize(swapchain const& sc) { return mBackend->clearPendingResize(sc.handle); }

tg::isize2 Context::get_backbuffer_size(swapchain const& sc) const { return mBackend->getBackbufferSize(sc.handle); }

phi::format Context::get_backbuffer_format(swapchain const& sc) const { return mBackend->getBackbufferFormat(sc.handle); }

unsigned Context::get_num_backbuffers(swapchain const& sc) const { return mBackend->getNumBackbuffers(sc.handle); }

unsigned Context::calculate_texture_upload_size(tg::isize3 size, phi::format fmt, unsigned num_mips) const
{
    return phi::util::get_texture_size_bytes(size, fmt, num_mips, mBackendType == pr::backend::d3d12);
}

unsigned Context::calculate_texture_pixel_offset(tg::isize2 size, format fmt, tg::ivec2 pixel) const
{
    return phi::util::get_texture_pixel_byte_offset(size, fmt, pixel, mBackendType == pr::backend::d3d12);
}

void Context::set_debug_name(const texture& tex, cc::string_view name) { mBackend->setDebugName(tex.res.handle, name); }
void Context::set_debug_name(const render_target& rt, cc::string_view name) { mBackend->setDebugName(rt.res.handle, name); }
void Context::set_debug_name(const buffer& buf, cc::string_view name) { mBackend->setDebugName(buf.res.handle, name); }
void Context::set_debug_name(phi::handle::resource raw_res, cc::string_view name) { mBackend->setDebugName(raw_res, name); }

render_target Context::acquire_backbuffer(swapchain const& sc)
{
    auto const backbuffer = mBackend->acquireBackbuffer(sc.handle);
    auto const size = mBackend->getBackbufferSize(sc.handle);
#ifdef CC_ENABLE_ASSERTIONS
    mSafetyState.did_acquire_before_present = true;
#endif
    return {{backbuffer, backbuffer.is_valid() ? acquireGuid() : 0}, {mBackend->getBackbufferFormat(sc.handle), size.width, size.height, 1, 1, {0, 0, 0, 1}}};
}

unsigned Context::clear_resource_caches()
{
    cc::vector<phi::handle::resource> freeable;
    freeable.reserve(100);

    auto const gpu_epoch = mGpuEpochTracker._cached_epoch_gpu;

    mCacheRenderTargets.cull_all(gpu_epoch, [&](phi::handle::resource rt) { freeable.push_back(rt); });
    mCacheTextures.cull_all(gpu_epoch, [&](phi::handle::resource rt) { freeable.push_back(rt); });
    mCacheBuffers.cull_all(gpu_epoch, [&](phi::handle::resource rt) { freeable.push_back(rt); });

    mBackend->freeRange(freeable);
    return unsigned(freeable.size());
}

unsigned Context::clear_shader_view_cache()
{
    cc::vector<phi::handle::shader_view> freeable;
    freeable.reserve(100);

    auto const gpu_epoch = mGpuEpochTracker._cached_epoch_gpu;

    mCacheGraphicsSVs.cull_all(gpu_epoch, [&](phi::handle::shader_view sv) { freeable.push_back(sv); });
    mCacheComputeSVs.cull_all(gpu_epoch, [&](phi::handle::shader_view sv) { freeable.push_back(sv); });

    mBackend->freeRange(freeable);
    return unsigned(freeable.size());
}

unsigned Context::clear_pipeline_state_cache()
{
    unsigned num_frees = 0;
    auto const gpu_epoch = mGpuEpochTracker._cached_epoch_gpu;

    mCacheGraphicsPSOs.cull_all(gpu_epoch, [&](phi::handle::pipeline_state pso) {
        ++num_frees;
        mBackend->free(pso);
    });
    mCacheComputePSOs.cull_all(gpu_epoch, [&](phi::handle::pipeline_state pso) {
        ++num_frees;
        mBackend->free(pso);
    });

    return num_frees;
}

unsigned Context::clear_pending_deferred_frees() { return mDeferredQueue.free_all_pending(*this); }

void Context::initialize(backend type, cc::allocator* alloc)
{
    phi::backend_config cfg;
#ifndef CC_RELEASE
    cfg.validation = phi::validation_level::on_extended;
#endif
    initialize(type, cfg, alloc);
}

void Context::initialize(backend type, const phi::backend_config& config, cc::allocator* alloc)
{
    CC_RUNTIME_ASSERT(mBackend == nullptr && "pr::Context double initialize");

    mOwnsBackend = true;
    mBackend = detail::make_backend(type);
    mBackend->initialize(config);
    CC_RUNTIME_ASSERT(mBackend != nullptr && "Failed to create backend");
    internalInitialize(alloc);
}

void Context::initialize(phi::Backend* backend, cc::allocator* alloc)
{
    CC_RUNTIME_ASSERT(mBackend == nullptr && "pr::Context double initialize");

    mBackend = backend;
    mOwnsBackend = false;
    CC_RUNTIME_ASSERT(mBackend != nullptr && "Invalid backend received");
    internalInitialize(alloc);
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

        // empty all caches, this could be way optimized
        mCacheGraphicsPSOs.iterate_values([&](phi::handle::pipeline_state pso) { mBackend->free(pso); });
        mCacheComputePSOs.iterate_values([&](phi::handle::pipeline_state pso) { mBackend->free(pso); });
        mCacheGraphicsSVs.iterate_values([&](phi::handle::shader_view sv) { mBackend->free(sv); });
        mCacheComputeSVs.iterate_values([&](phi::handle::shader_view sv) { mBackend->free(sv); });

        mCacheBuffers.iterate_values([&](phi::handle::resource res) { mBackend->free(res); });
        mCacheTextures.iterate_values([&](phi::handle::resource res) { mBackend->free(res); });
        mCacheRenderTargets.iterate_values([&](phi::handle::resource res) { mBackend->free(res); });

        // destroy other components
        mGpuEpochTracker.destroy(mBackend);
        mShaderCompiler.destroy();
        mDeferredQueue.destroy(*this);

        // if onwing mBackend, destroy and free it
        if (mOwnsBackend)
        {
            mBackend->destroy();
            delete mBackend;
        }

        mBackend = nullptr;
    }
}

void Context::internalInitialize(cc::allocator* alloc)
{
    mGpuEpochTracker.initialize(mBackend);
    mCacheBuffers.reserve(256);
    mCacheTextures.reserve(256);
    mCacheRenderTargets.reserve(64);
    mShaderCompiler.initialize();
    mDeferredQueue.initialize(alloc);

    mGPUTimestampFrequency = mBackend->getGPUTimestampFrequency();
    mBackendType = mBackend->getBackendType() == phi::backend_type::d3d12 ? pr::backend::d3d12 : pr::backend::vulkan;
}

render_target Context::createRenderTarget(const render_target_info& info, char const* dbg_name)
{
    return {{mBackend->createRenderTargetFromInfo(info, dbg_name), acquireGuid()}, info};
}

texture Context::createTexture(const texture_info& info, const char* dbg_name)
{
    return {{mBackend->createTextureFromInfo(info, dbg_name), acquireGuid()}, info};
}

buffer Context::createBuffer(const buffer_info& info, char const* dbg_name)
{
    CC_ASSERT((info.allow_uav ? info.heap == phi::resource_heap::gpu : true) && "mapped buffers cannot be created with UAV support");

    phi::handle::resource handle = mBackend->createBufferFromInfo(info, dbg_name);
    return {{handle, acquireGuid()}, info};
}

render_target Context::acquireRenderTarget(const render_target_info& info)
{
    auto lookup = mCacheRenderTargets.acquire(info, mGpuEpochTracker._cached_epoch_gpu);
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
    auto lookup = mCacheTextures.acquire(info, mGpuEpochTracker._cached_epoch_gpu);
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
    auto lookup = mCacheBuffers.acquire(info, mGpuEpochTracker._cached_epoch_gpu);
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
    dxcw::destroy_blob(blob); // intern. synced
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

phi::handle::pipeline_state Context::acquire_graphics_pso(cc::hash_t hash, graphics_pass_info const& gp, framebuffer_info const& fb)
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

phi::handle::pipeline_state Context::acquire_compute_pso(cc::hash_t hash, const compute_pass_info& cp)
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

phi::handle::shader_view Context::acquire_graphics_sv(cc::hash_t hash, const hashable_storage<shader_view_info>& info_storage)
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

phi::handle::shader_view Context::acquire_compute_sv(cc::hash_t hash, const hashable_storage<shader_view_info>& info_storage)
{
    phi::handle::shader_view sv = mCacheComputeSVs.acquire(hash);
    if (!sv.is_valid())
    {
        shader_view_info const& info = info_storage.get();
        sv = mBackend->createShaderView(info.srvs, info.uavs, info.samplers, true);
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

void Context::free_graphics_pso(cc::hash_t hash) { mCacheGraphicsPSOs.free(hash, mGpuEpochTracker.get_current_epoch_cpu()); }
void Context::free_compute_pso(cc::hash_t hash) { mCacheComputePSOs.free(hash, mGpuEpochTracker.get_current_epoch_cpu()); }

void Context::free_graphics_sv(cc::hash_t hash) { mCacheGraphicsSVs.free(hash, mGpuEpochTracker.get_current_epoch_cpu()); }
void Context::free_compute_sv(cc::hash_t hash) { mCacheComputeSVs.free(hash, mGpuEpochTracker.get_current_epoch_cpu()); }

auto_buffer pr::Context::make_upload_buffer_for_texture(const texture& tex, unsigned num_mips, const char* debug_name)
{
    return make_upload_buffer(calculate_texture_upload_size(tex, num_mips), 0, debug_name);
}

unsigned pr::Context::calculate_texture_upload_size(const texture& texture, unsigned num_mips) const
{
    return calculate_texture_upload_size({texture.info.width, texture.info.height, int(texture.info.depth_or_array_size)}, texture.info.fmt, num_mips);
}

unsigned Context::calculate_texture_upload_size(const render_target& target) const
{
    return calculate_texture_upload_size({target.info.width, target.info.height, int(target.info.array_size)}, target.info.format, 1);
}

unsigned pr::Context::calculate_texture_upload_size(int width, format fmt, unsigned num_mips) const
{
    return calculate_texture_upload_size({width, 1, 1}, fmt, num_mips);
}

unsigned pr::Context::calculate_texture_upload_size(tg::isize2 size, format fmt, unsigned num_mips) const
{
    return calculate_texture_upload_size({size.width, size.height, 1}, fmt, num_mips);
}
