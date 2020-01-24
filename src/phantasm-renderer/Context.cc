#include "Context.hh"

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/config.hh>

#include <phantasm-renderer/Frame.hh>
#include <phantasm-renderer/backends.hh>

using namespace pr;

Frame Context::make_frame()
{
    return {}; // TODO
}

void Context::submit(const Frame& frame)
{
    // TODO
}

void Context::submit(const CompiledFrame& frame)
{
    // TODO
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
    mCacheShaderViews.reserve(1024);
    mCacheUploadBuffers.reserve(256);
    mCachePipelineStates.reserve(64);
    mCachePipelineStatesCompute.reserve(64);
}

Context::~Context() { mGpuEpochTracker.destroy(); }
