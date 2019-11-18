#include "Context.hh"

#include <phantasm-renderer/Frame.hh>
#include <phantasm-renderer/backend/vulkan/BackendVulkan.hh>
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

Context::Context()
{
    backend::backend_config cfg;
#ifndef CC_RELEASE
    cfg.validation = backend::validation_level::on;
#endif
    // mBackend = make_vulkan_backend(cfg);
}

Context::Context(cc::poly_unique_ptr<Backend> backend) : mBackend(std::move(backend)) {}

Context::~Context() = default; // here because of poly_unique_ptr
