#pragma once
#ifdef PR_BACKEND_VULKAN

#include <phantasm-renderer/backend/BackendInterface.hh>

namespace pr::backend::vk
{
class BackendVulkan final : public BackendInterface
{
public:
    void initialize();
};
}

#endif
