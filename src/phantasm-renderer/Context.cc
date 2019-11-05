#include "Context.hh"

#include <phantasm-renderer/backend/vulkan/BackendVulkan.hh>
#include <phantasm-renderer/backends.hh>

using namespace pr;

Context::Context()
{
    backend::vk::vulkan_config cfg;
#ifndef CC_RELEASE
    cfg.enable_validation = true;
#endif
    mBackend = make_vulkan_backend(cfg);
}

Context::Context(cc::poly_unique_ptr<Backend> backend) : mBackend(std::move(backend)) {}
