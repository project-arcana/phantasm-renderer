#include "CommandBufferRing.hh"

#include "Device.hh"
#include "common/verify.hh"
#include "common/zero_struct.hh"

pr::backend::vk::CommandBufferRing::~CommandBufferRing()
{
    for (auto& ring_entry : mRing)
    {
        vkFreeCommandBuffers(mDevice, ring_entry.command_pool, ring_entry.command_buffers.size(), ring_entry.command_buffers.data());
        vkDestroyCommandPool(mDevice, ring_entry.command_pool, nullptr);
    }
}

void pr::backend::vk::CommandBufferRing::initialize(pr::backend::vk::Device& device, unsigned num_allocators, unsigned num_command_lists_per_allocator, int queue_family_index)
{
    mDevice = device.getDevice();

    mRing = cc::array<ring_entry>::defaulted(num_allocators);

    for (auto& ring_entry : mRing)
    {
        // pool
        {
            VkCommandPoolCreateInfo info;
            zero_info_struct(info, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
            info.pNext = nullptr;
            info.queueFamilyIndex = unsigned(queue_family_index);
            info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

            PR_VK_VERIFY_SUCCESS(vkCreateCommandPool(mDevice, &info, nullptr, &ring_entry.command_pool));
        }

        // buffers
        ring_entry.command_buffers = decltype(ring_entry.command_buffers)::uninitialized(num_command_lists_per_allocator);
        {
            VkCommandBufferAllocateInfo info;
            zero_info_struct(info, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
            info.pNext = nullptr;
            info.commandPool = ring_entry.command_pool;
            info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            info.commandBufferCount = unsigned(ring_entry.command_buffers.size());

            PR_VK_VERIFY_SUCCESS(vkAllocateCommandBuffers(mDevice, &info, ring_entry.command_buffers.data()));
        }

        ring_entry.num_command_buffers_in_flight = 0;
    }

    mRingIndex = 0;
    mActiveEntry = &mRing[unsigned(mRingIndex)];
}

void pr::backend::vk::CommandBufferRing::onBeginFrame()
{
    ++mRingIndex;
    // fast %
    if (mRingIndex >= int(mRing.size()))
        mRingIndex -= int(mRing.size());

    mActiveEntry = &mRing[unsigned(mRingIndex)];

    // reset the entry
    mActiveEntry->reset();
}

VkCommandBuffer pr::backend::vk::CommandBufferRing::acquireCommandBuffer()
{
    auto res = mActiveEntry->acquire_buffer();

    VkCommandBufferBeginInfo info;
    zero_info_struct(info, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
    info.pNext = nullptr;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.pInheritanceInfo = nullptr;

    PR_VK_VERIFY_SUCCESS(vkBeginCommandBuffer(res, &info));

    return res;
}

VkCommandBuffer pr::backend::vk::CommandBufferRing::ring_entry::acquire_buffer()
{
    // No need to assert, cc::array internally asserts
    return command_buffers[unsigned(num_command_buffers_in_flight++)];
}

void pr::backend::vk::CommandBufferRing::ring_entry::reset() { num_command_buffers_in_flight = 0; }
