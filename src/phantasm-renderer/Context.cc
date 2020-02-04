#include "Context.hh"

#include <phantasm-renderer/Frame.hh>
#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/config.hh>
#include <phantasm-renderer/backends.hh>
#include <phantasm-renderer/backend/Backend.hh>

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
}

Context::Context(cc::poly_unique_ptr<phi::Backend> backend) : mBackend(std::move(backend)) {}

Context::~Context() = default; // here because of poly_unique_ptr
