#pragma once

#include <clean-core/capped_array.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
class Device;
struct gpu_information;

class Swapchain
{
public:
    void initialize(Device const& device, VkSurfaceKHR surface, unsigned num_backbuffers, int w, int h);

    void destroy();

public:
    void onResize(int width, int height);

    void performPresentSubmit(VkCommandBuffer command_buf);
    void present();

    void waitForBackbuffer();


public:
    [[nodiscard]] VkFormat getBackbufferFormat() const { return mBackbufferFormat.format; }
    [[nodiscard]] VkExtent2D getBackbufferExtent() const { return mBackbufferExtent; }
    [[nodiscard]] unsigned getNumBackbuffers() const { return unsigned(mBackbuffers.size()); }

    [[nodiscard]] VkRenderPass getRenderPass() const { return mRenderPass; }

    [[nodiscard]] unsigned getCurrentBackbufferIndex() const { return mActiveImageIndex; }
    [[nodiscard]] VkImage getCurrentBackbuffer() const { return mBackbuffers[mActiveImageIndex].image; }
    [[nodiscard]] VkImageView getCurrentBackbufferView() const { return mBackbuffers[mActiveImageIndex].view; }
    [[nodiscard]] VkFramebuffer getCurrentFramebuffer() const { return mBackbuffers[mActiveImageIndex].framebuffer; }

    [[nodiscard]] VkFramebuffer getFramebuffer(unsigned i) const { return mBackbuffers[i].framebuffer; }

    [[nodiscard]] VkSwapchainKHR getSwapchain() const { return mSwapchain; }
    //[[nodiscard]] auto const& getBackbufferViews() const { return mBackbufferViews; }

private:
    void createSwapchain(int w, int h);
    void destroySwapchain();

private:
    static auto constexpr max_num_backbuffers = 6u;

    // non-owning
    VkSurfaceKHR mSurface;
    VkPhysicalDevice mPhysicalDevice;
    VkDevice mDevice;
    VkQueue mPresentQueue;

    // owning
    VkSwapchainKHR mSwapchain;
    VkRenderPass mRenderPass;

    struct backbuffer
    {
        // sync objects
        VkFence fence_command_buf_executed;
        VkSemaphore sem_image_available;
        VkSemaphore sem_render_finished;

        // viewport-dependent resources
        VkImage image;
        VkImageView view;
        VkFramebuffer framebuffer;
    };

    cc::capped_array<backbuffer, max_num_backbuffers> mBackbuffers;

    unsigned mActiveFenceIndex = 0;
    unsigned mActiveImageIndex = 0;

    VkSurfaceFormatKHR mBackbufferFormat;
    VkExtent2D mBackbufferExtent;
};

}
