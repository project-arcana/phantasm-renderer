#pragma once

#include <phantasm-renderer/backend/Backend.hh>

#include "vulkan_config.hh"

namespace pr
{
class VulkanBackend : public Backend
{
public:
    vulkan_config const& config() const { return mConfig; }

    VulkanBackend(vulkan_config const& cfg = {}) : mConfig(cfg) {}

private:
    vulkan_config mConfig;
};
}
