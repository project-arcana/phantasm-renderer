#pragma once
#ifdef PR_BACKEND_VULKAN

#include <vector>

#include <phantasm-renderer/backend/BackendInterface.hh>

namespace pr::backend::vk
{
class BackendVulkan final : public BackendInterface
{
public:
    void initialize();

private:
    void getLayersAndExtensions(std::vector<char const*>& out_extensions, std::vector<char const*>& out_layers);
};
}

#endif
