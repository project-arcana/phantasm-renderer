#pragma once

#include <clean-core/capped_array.hh>

#include "loader/volk.hh"

namespace pr::backend::vk {
class Device;
struct gpu_information;

class Swapchain
{
public:
    void initialize(Device const& device, gpu_information const& gpu_info, VkSurfaceKHR surface);

    void destroy();

private:
    void queryBackbuffers();

private:
    static auto constexpr max_num_backbuffers = 6u;

    // non-owning
    VkSurfaceKHR mSurface;
    VkDevice mDevice;

    // owning
    VkSwapchainKHR mSwapchain;

    cc::capped_array<VkImage, max_num_backbuffers> mBackbuffers;
    cc::capped_array<VkImageView, max_num_backbuffers> mBackbufferViews;
    VkFormat mBackbufferFormat;
    VkExtent2D mBackbufferExtent;
};

}
