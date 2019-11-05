#pragma once
#ifdef PR_BACKEND_VULKAN

#include <vector>

#include <phantasm-renderer/backend/BackendInterface.hh>

#include "loader/volk.hh"
#include "vulkan_config.hh"

namespace pr::backend::vk
{
class BackendVulkan final : public BackendInterface
{
public:
    void initialize(vulkan_config const& config);

    ~BackendVulkan() override;

private:
    VkInstance mInstance;
};
}

#endif
