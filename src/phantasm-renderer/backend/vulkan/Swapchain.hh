#pragma once

#include <clean-core/capped_array.hh>

#include <typed-geometry/tg-lean.hh>

#include <phantasm-renderer/backend/types.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
class Device;
struct gpu_information;

class Swapchain
{
public:
    void initialize(Device const& device, VkSurfaceKHR surface, unsigned num_backbuffers, int w, int h, present_mode sync);

    void destroy();

public:
    /// flushes the device and recreates the swapchain, and all associated resources
    void onResize(int width_hint, int height_hint);

    /// fill out a VkSubmitInfo struct with the necessary semaphores,
    /// then submit an internal dummy command buffer with the necessary fence
    void performPresentSubmit();

    /// present the active backbuffer to the screen
    /// can trigger a resize instead of presenting if the swapchain is stale
    void present();

    /// wait for the next backbuffer
    /// this has to be called before calls to getCurrent<X>
    /// if this returns false, the backbuffer has resized, and the frame should likely be discarded
    [[nodiscard]] bool waitForBackbuffer();

    [[nodiscard]] bool hasBackbufferResized() { return mBackbufferHasResized; }
    void clearBackbufferResizeFlag() { mBackbufferHasResized = false; }

public:
    [[nodiscard]] VkFormat getBackbufferFormat() const { return mBackbufferFormat.format; }
    [[nodiscard]] tg::ivec2 getBackbufferSize() const { return mBackbufferSize; }
    [[nodiscard]] unsigned getNumBackbuffers() const { return unsigned(mBackbuffers.size()); }

    [[nodiscard]] VkRenderPass getRenderPass() const { return mRenderPass; }

    [[nodiscard]] unsigned getCurrentBackbufferIndex() const { return mActiveImageIndex; }
    [[nodiscard]] VkImage getCurrentBackbuffer() const { return mBackbuffers[mActiveImageIndex].image; }
    [[nodiscard]] VkImageView getCurrentBackbufferView() const { return mBackbuffers[mActiveImageIndex].view; }
    [[nodiscard]] VkFramebuffer getCurrentFramebuffer() const { return mBackbuffers[mActiveImageIndex].framebuffer; }

    [[nodiscard]] VkFramebuffer getFramebuffer(unsigned i) const { return mBackbuffers[i].framebuffer; }

    [[nodiscard]] VkSwapchainKHR getSwapchain() const { return mSwapchain; }

private:
    void createSwapchain(int width_hint, int height_hint);
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
    VkCommandPool mDummyPresentCommandPool;

    struct backbuffer
    {
        // sync objects
        /// reset and signalled in ::performPresentSubmit, waited on (CPU) in ::present
        VkFence fence_command_buf_executed;
        /// signalled in ::waitForBackbuffer, waited on (GPU) in ::performPresentSubmit
        VkSemaphore sem_image_available;
        /// signalled in ::performPresentSubmit, waited on (GPU) in ::present
        VkSemaphore sem_render_finished;

        // dummy present command buffer
        VkCommandBuffer dummy_present_cmdbuf;

        // viewport-dependent resources
        VkImage image;
        VkImageView view;
        VkFramebuffer framebuffer;
    };

    cc::capped_array<backbuffer, max_num_backbuffers> mBackbuffers;

    unsigned mActiveFenceIndex = 0;
    unsigned mActiveImageIndex = 0;

    VkSurfaceFormatKHR mBackbufferFormat;

    tg::ivec2 mBackbufferSize;
    bool mBackbufferHasResized = true;

    present_mode mSyncMode;
};

}
