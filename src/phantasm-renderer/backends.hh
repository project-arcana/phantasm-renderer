#pragma once

#include <clean-core/fwd.hh>

#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/backend/vulkan/vulkan_config.hh>

namespace pr
{
cc::poly_unique_ptr<VulkanBackend> make_vulkan_backend(vulkan_config const& cfg = {});
}
