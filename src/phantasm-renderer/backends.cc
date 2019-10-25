#include "backends.hh"

#include <clean-core/poly_unique_ptr.hh>

#include <phantasm-renderer/backend/vulkan/VulkanBackend.hh>

cc::poly_unique_ptr<pr::VulkanBackend> pr::make_vulkan_backend(vulkan_config const& cfg) { return cc::make_poly_unique<VulkanBackend>(cfg); }
