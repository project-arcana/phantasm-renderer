#pragma once

#include <clean-core/array.hh>

#include "loader/volk.hh"

namespace pr::backend::vk
{
class Device;

class CommandBufferRing
{
public:
    ~CommandBufferRing();

    void initialize(Device& device, unsigned num_allocators, unsigned num_command_lists_per_allocator, int queue_family_index);

    /// Steps forward in the ring, resetting the next allocator
    void onBeginFrame();

    /// Acquires a command buffer, vkBeginCommandBuffer has already been called
    [[nodiscard]] VkCommandBuffer acquireCommandBuffer();

private:
    // non-owning
    VkDevice mDevice;

    // owning
    struct ring_entry
    {
        VkCommandPool command_pool;
        cc::array<VkCommandBuffer> command_buffers;
        int num_command_buffers_in_flight = 0;

        [[nodiscard]] VkCommandBuffer acquire_buffer();
        void reset();
    };

    cc::array<ring_entry> mRing;
    int mRingIndex = 0;
    ring_entry* mActiveEntry = nullptr;
};

}
