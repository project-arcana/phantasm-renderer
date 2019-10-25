#include "Context.hh"

#include <phantasm-renderer/backend/vulkan/VulkanBackend.hh>

using namespace pr;

Context::Context()
{
    vulkan_config cfg;
#ifndef CC_RELEASE
    cfg.enable_validation = true;
#endif
    mBackend = cc::make_poly_unique<VulkanBackend>(cfg);
}

Context::Context(cc::poly_unique_ptr<Backend> backend) : mBackend(std::move(backend)) {}
