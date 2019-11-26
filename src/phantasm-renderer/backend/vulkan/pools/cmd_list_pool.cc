#include "cmd_list_pool.hh"

#include <phantasm-renderer/backend/detail/flat_map.hh>
#include <phantasm-renderer/backend/vulkan/BackendVulkan.hh>

void pr::backend::vk::cmd_allocator_node::initialize(VkDevice device, int num_cmd_lists, unsigned queue_family_index, FenceRingbuffer* fence_ring)
{
    _fence_ring = fence_ring;

    // create pool
    {
        VkCommandPoolCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.queueFamilyIndex = queue_family_index;
        info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        PR_VK_VERIFY_SUCCESS(vkCreateCommandPool(device, &info, nullptr, &_cmd_pool));
    }
    // allocate buffers
    {
        _cmd_buffers = _cmd_buffers.uninitialized(unsigned(num_cmd_lists));

        VkCommandBufferAllocateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = _cmd_pool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = unsigned(num_cmd_lists);

        PR_VK_VERIFY_SUCCESS(vkAllocateCommandBuffers(device, &info, _cmd_buffers.data()));
    }
}

void pr::backend::vk::cmd_allocator_node::destroy(VkDevice device) { vkDestroyCommandPool(device, _cmd_pool, nullptr); }

VkCommandBuffer pr::backend::vk::cmd_allocator_node::acquire(VkDevice device)
{
    if (is_full())
    {
        // the allocator is full, we are almost dead but might be able to reset
        auto const reset_success = try_reset_blocking(device);
        CC_RUNTIME_ASSERT(reset_success && "cmdlist allocator node overcommitted and unable to recover");
        // we were able to recover, but should warn even now
    }

    auto const res = _cmd_buffers[_num_in_flight];
    ++_num_in_flight;

    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    PR_VK_VERIFY_SUCCESS(vkBeginCommandBuffer(res, &info));

    return res;
}

void pr::backend::vk::cmd_allocator_node::on_submit(unsigned num, unsigned fence_index)
{
    // first, update the latest fence
    auto const previous_fence = _latest_fence.exchange(fence_index);
    if (previous_fence != unsigned(-1))
    {
        // release previous fence
        _fence_ring->decrementRefcount(previous_fence);
    }

    // second, increment the pending execution counter, as it guards access to _latest_fence
    // (an increment here might turn is_submit_counter_up_to_date true)
    _num_pending_execution.fetch_add(num);
}

bool pr::backend::vk::cmd_allocator_node::try_reset(VkDevice device)
{
    if (can_reset())
    {
        // full, and all acquired cmdbufs have been either submitted or discarded, check the fences

        if (_num_pending_execution.load() > 0)
        {
            // there was at least a single real submission, load the latest fance
            auto const relevant_fence = _latest_fence.load();
            CC_ASSERT(relevant_fence != unsigned(-1));

            if (_fence_ring->isFenceSignalled(device, relevant_fence))
            {
                // the fence is signalled, we can reset

                // decrement the refcount on the fence and reset the latest fence index
                _fence_ring->decrementRefcount(relevant_fence);
                _latest_fence.store(unsigned(-1));

                // perform the reset
                do_reset(device);
                return true;
            }
            else
            {
                // some fences are pending
                return false;
            }
        }
        else
        {
            // all cmdbuffers were discarded, we can reset unconditionally
            do_reset(device);
            return true;
        }
    }
    else
    {
        // can't reset
        return !is_full();
    }
}

bool pr::backend::vk::cmd_allocator_node::try_reset_blocking(VkDevice device)
{
    if (can_reset())
    {
        // full, and all acquired cmdbufs have been either submitted or discarded, check the fences

        if (_num_pending_execution.load() > 0)
        {
            // there was at least a single real submission, load the latest fance
            auto const relevant_fence = _latest_fence.load();
            CC_ASSERT(relevant_fence != unsigned(-1));

            // block
            _fence_ring->waitForFence(device, relevant_fence);

            // decrement the refcount on the fence and reset the latest fence index
            _fence_ring->decrementRefcount(relevant_fence);
            _latest_fence.store(unsigned(-1));
        }

        do_reset(device);
        return true;
    }
    else
    {
        // can't reset
        return !is_full();
    }
}

void pr::backend::vk::cmd_allocator_node::do_reset(VkDevice device)
{
    PR_VK_VERIFY_SUCCESS(vkResetCommandPool(device, _cmd_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));
    _num_in_flight = 0;
    _num_discarded = 0;
    _num_pending_execution = 0;
}

void pr::backend::vk::FenceRingbuffer::initialize(VkDevice device, unsigned num_fences)
{
    mFences = mFences.defaulted(num_fences);

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    for (auto& fence : mFences)
    {
        fence.ref_count.store(0);
        PR_VK_VERIFY_SUCCESS(vkCreateFence(device, &fence_info, nullptr, &fence.raw_fence));
    }
}

void pr::backend::vk::FenceRingbuffer::destroy(VkDevice device)
{
    for (auto& fence : mFences)
        vkDestroyFence(device, fence.raw_fence, nullptr);
}

unsigned pr::backend::vk::FenceRingbuffer::acquireFence(VkDevice device, VkFence& out_fence)
{
    for (auto i = 0u; i < mFences.size(); ++i)
    {
        auto const fence_i = mNextFence;

        ++mNextFence;
        if (mNextFence >= mFences.size())
            mNextFence -= static_cast<unsigned>(mFences.size());

        auto& node = mFences[fence_i];

        if (node.ref_count.load() == 0)
        {
            vkResetFences(device, 1, &node.raw_fence);
            node.ref_count.store(1); // set the refcount to one
            out_fence = node.raw_fence;
            return fence_i;
        }
    }

    // none of the fences are resettable
    CC_RUNTIME_ASSERT(false && "Fence ringbuffer is full");
    return 0;
}

void pr::backend::vk::FenceRingbuffer::waitForFence(VkDevice device, unsigned index) const
{
    auto& node = mFences[index];
    auto const vkres = vkWaitForFences(device, 1, &node.raw_fence, VK_TRUE, UINT64_MAX);
    CC_ASSERT(vkres == VK_SUCCESS); // other cases are TIMEOUT (2^64 ns > 584 years) or DEVICE_LOST (dead anyway)
}

void pr::backend::vk::CommandAllocatorBundle::initialize(
    VkDevice device, int num_allocators, int num_cmdlists_per_allocator, unsigned queue_family_index, pr::backend::vk::FenceRingbuffer* fence_ring)
{
    mAllocators = mAllocators.defaulted(static_cast<size_t>(num_allocators));
    mActiveAllocator = 0u;

    for (cmd_allocator_node& alloc_node : mAllocators)
    {
        alloc_node.initialize(device, num_cmdlists_per_allocator, queue_family_index, fence_ring);
    }
}

void pr::backend::vk::CommandAllocatorBundle::destroy(VkDevice device)
{
    for (auto& alloc_node : mAllocators)
        alloc_node.destroy(device);
}

pr::backend::vk::cmd_allocator_node* pr::backend::vk::CommandAllocatorBundle::acquireMemory(VkDevice device, VkCommandBuffer& out_buffer)
{
    updateActiveIndex(device);
    auto& active_alloc = mAllocators[mActiveAllocator];
    out_buffer = active_alloc.acquire(device);
    return &active_alloc;
}

void pr::backend::vk::CommandAllocatorBundle::updateActiveIndex(VkDevice device)
{
    for (auto it = 0u; it < mAllocators.size(); ++it)
    {
        if (!mAllocators[mActiveAllocator].is_full() || mAllocators[mActiveAllocator].try_reset(device))
            // not full, or nonblocking reset successful
            return;
        else
        {
            ++mActiveAllocator;
            // fast %
            if (mActiveAllocator >= mAllocators.size())
                mActiveAllocator -= mAllocators.size();
        }
    }

    // all non-blocking resets failed, try blocking now
    for (auto it = 0u; it < mAllocators.size(); ++it)
    {
        if (mAllocators[mActiveAllocator].try_reset_blocking(device))
            // blocking reset successful
            return;
        else
        {
            ++mActiveAllocator;
            // fast %
            if (mActiveAllocator >= mAllocators.size())
                mActiveAllocator -= mAllocators.size();
        }
    }

    // all allocators have at least 1 dangling cmdlist, we cannot recover
    CC_RUNTIME_ASSERT(false && "all allocators overcommitted and unresettable");
}

pr::backend::handle::command_list pr::backend::vk::CommandListPool::create(VkCommandBuffer& out_cmdlist, pr::backend::vk::CommandAllocatorBundle& thread_allocator)
{
    unsigned res_handle;
    {
        auto lg = std::lock_guard(mMutex);
        res_handle = mPool.acquire();
    }

    cmd_list_node& new_node = mPool.get(res_handle);
    new_node.responsible_allocator = thread_allocator.acquireMemory(mDevice, new_node.raw_buffer);

    out_cmdlist = new_node.raw_buffer;
    return {static_cast<handle::index_t>(res_handle)};
}

void pr::backend::vk::CommandListPool::freeOnSubmit(pr::backend::handle::command_list cl, unsigned fence_index)
{
    cmd_list_node& freed_node = mPool.get(static_cast<unsigned>(cl.index));
    {
        auto lg = std::lock_guard(mMutex);
        freed_node.responsible_allocator->on_submit(1, fence_index);
        mPool.release(static_cast<unsigned>(cl.index));
    }
}

void pr::backend::vk::CommandListPool::freeOnSubmit(cc::span<const pr::backend::handle::command_list> cls, unsigned fence_index)
{
    backend::detail::capped_flat_map<cmd_allocator_node*, unsigned, 24> unique_allocators;

    // free the cls in the pool and gather the unique allocators
    {
        auto lg = std::lock_guard(mMutex);
        for (auto const& cl : cls)
        {
            if (cl == handle::null_command_list)
                continue;

            cmd_list_node& freed_node = mPool.get(static_cast<unsigned>(cl.index));
            unique_allocators.get_value(freed_node.responsible_allocator, 0u) += 1;
            mPool.release(static_cast<unsigned>(cl.index));
        }
    }

    if (!unique_allocators._nodes.empty())
    {
        // the given fence_index has a reference count of 1, increment it to the amount of unique allocators responsible
        mFenceRing.incrementRefcount(fence_index, int(unique_allocators._nodes.size()) - 1);

        // notify all unique allocators
        for (auto const& unique_alloc : unique_allocators._nodes)
        {
            unique_alloc.key->on_submit(unique_alloc.val, fence_index);
        }
    }
}

void pr::backend::vk::CommandListPool::freeOnDiscard(pr::backend::handle::command_list cl)
{
    cmd_list_node& freed_node = mPool.get(static_cast<unsigned>(cl.index));
    {
        auto lg = std::lock_guard(mMutex);
        freed_node.responsible_allocator->on_discard();
        mPool.release(static_cast<unsigned>(cl.index));
    }
}

void pr::backend::vk::CommandListPool::initialize(pr::backend::vk::BackendVulkan& backend,
                                                  int num_allocators_per_thread,
                                                  int num_cmdlists_per_allocator,
                                                  cc::span<pr::backend::vk::CommandAllocatorBundle*> thread_allocators)
{
    mDevice = backend.mDevice.getDevice();

    auto const num_cmdlists_per_thread = static_cast<unsigned>(num_allocators_per_thread * num_cmdlists_per_allocator);
    auto const num_cmdlists_total = num_cmdlists_per_thread * thread_allocators.size();
    auto const num_allocators_total = static_cast<unsigned>(num_allocators_per_thread) * static_cast<unsigned>(thread_allocators.size());

    mPool.initialize(num_cmdlists_total);
    mFenceRing.initialize(mDevice, num_allocators_total + 5); // arbitrary safety buffer, should never be required


    auto const graphics_queue_family = static_cast<unsigned>(backend.mDevice.getQueueFamilyGraphics());
    for (auto i = 0u; i < thread_allocators.size(); ++i)
    {
        thread_allocators[i]->initialize(mDevice, num_allocators_per_thread, num_cmdlists_per_allocator, graphics_queue_family, &mFenceRing);
    }

    // Flush the backend
    backend.flushGPU();
}

void pr::backend::vk::CommandListPool::destroy() { mFenceRing.destroy(mDevice); }