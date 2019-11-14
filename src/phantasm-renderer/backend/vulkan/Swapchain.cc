#include "Swapchain.hh"

#include "Device.hh"
#include "common/verify.hh"
#include "common/zero_struct.hh"
#include "gpu_choice_util.hh"

void pr::backend::vk::Swapchain::initialize(const pr::backend::vk::Device& device, gpu_information const& gpu_info, VkSurfaceKHR surface)
{
    mSurface = surface;
    mDevice = device.getDevice();

    auto num_backbuffers = gpu_info.surface_capabilities.minImageCount + 1;
    if (gpu_info.surface_capabilities.maxImageCount != 0 && num_backbuffers > gpu_info.surface_capabilities.maxImageCount)
        num_backbuffers = gpu_info.surface_capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR swapchain_info;
    zero_info_struct(swapchain_info, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
    swapchain_info.surface = surface;
    swapchain_info.minImageCount = num_backbuffers;

    auto const surface_format = choose_backbuffer_format(gpu_info.backbuffer_formats);
    mBackbufferFormat = surface_format.format;
    swapchain_info.imageFormat = surface_format.format;
    swapchain_info.imageColorSpace = surface_format.colorSpace;

    mBackbufferExtent = get_swap_extent(gpu_info.surface_capabilities, VkExtent2D{1280, 720});

    swapchain_info.imageExtent = mBackbufferExtent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // We require our graphics queue to also be able to present
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 0;
    swapchain_info.pQueueFamilyIndices = nullptr;

    swapchain_info.preTransform = gpu_info.surface_capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = choose_present_mode(gpu_info.present_modes, true);
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    PR_VK_VERIFY_SUCCESS(vkCreateSwapchainKHR(mDevice, &swapchain_info, nullptr, &mSwapchain));
    queryBackbuffers();
}

void pr::backend::vk::Swapchain::destroy()
{
    for (auto const view : mBackbufferViews)
        vkDestroyImageView(mDevice, view, nullptr);

    vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
}

void pr::backend::vk::Swapchain::queryBackbuffers()
{
    uint32_t num_backbuffers;
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &num_backbuffers, nullptr);
    CC_RUNTIME_ASSERT(num_backbuffers < max_num_backbuffers);
    mBackbuffers.emplace(num_backbuffers);
    vkGetSwapchainImagesKHR(mDevice, mSwapchain, &num_backbuffers, mBackbuffers.data());

    mBackbufferViews.emplace(num_backbuffers);
    for (auto i = 0u; i < num_backbuffers; ++i)
    {
        VkImageViewCreateInfo info;
        zero_info_struct(info, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        info.image = mBackbuffers[i];
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = mBackbufferFormat;
        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;
        PR_VK_VERIFY_SUCCESS(vkCreateImageView(mDevice, &info, nullptr, &mBackbufferViews[i]));
    }
}
