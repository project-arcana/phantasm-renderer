#pragma once

#include <atomic>
#include <mutex>

#include <clean-core/array.hh>

#include <phantasm-renderer/backend/detail/incomplete_state_cache.hh>
#include <phantasm-renderer/backend/detail/linked_pool.hh>
#include <phantasm-renderer/backend/vulkan/common/verify.hh>
#include <phantasm-renderer/backend/vulkan/loader/volk.hh>

namespace pr::backend::vk
{
class FenceRingbuffer
{
public:
    void initialize(VkDevice device, unsigned num_fences);
    void destroy(VkDevice device);

public:
    /// acquires a fence from the ringbuffer
    /// not thread safe, must not be called concurrently from multiple places
    /// the returned fence has an initial refcount of 1
    [[nodiscard]] unsigned acquireFence(VkDevice device, VkFence& out_fence);

    /// block until the fence at the given index is signalled
    /// thread safe
    void waitForFence(VkDevice device, unsigned index) const;

    /// returns true if the fence at the given index is signalled
    /// thread safe
    [[nodiscard]] bool isFenceSignalled(VkDevice device, unsigned index) const
    {
        return vkGetFenceStatus(device, mFences[index].raw_fence) == VK_SUCCESS;
    }

    /// increments the refcount of the fence at the given index
    /// thread safe
    void incrementRefcount(unsigned index, int amount = 1)
    {
        auto const pre_increment = mFences[index].ref_count.fetch_add(amount);
        CC_ASSERT(pre_increment >= 0);
    }

    /// decrements the refcount of the fence at the given index
    /// thread safe
    void decrementRefcount(unsigned index)
    {
        auto const pre_decrement = mFences[index].ref_count.fetch_sub(1);
        CC_ASSERT(pre_decrement >= 1);
    }

private:
    struct fence_node
    {
        VkFence raw_fence;
        /// the amount of allocators depending on this fence
        std::atomic_int ref_count;
    };

    cc::array<fence_node> mFences;
    unsigned mNextFence = 0;
};


struct cmd_allocator_node
{
public:
    void initialize(VkDevice device, int num_cmd_lists, unsigned queue_family_index, FenceRingbuffer* fence_ring);
    void destroy(VkDevice device);

public:
    /// returns true if this node is full
    [[nodiscard]] bool is_full() const { return _num_in_flight == _cmd_buffers.size(); }

    /// returns true if this node is full and capable of resetting
    [[nodiscard]] bool can_reset() const { return is_full() && is_submit_counter_up_to_date(); }

    /// acquire a command buffer from this allocator
    /// do not call if full (best case: blocking, worst case: crash)
    [[nodiscard]] VkCommandBuffer acquire(VkDevice device);

    /// to be called when a command buffer backed by this allocator
    /// is being dicarded (will never result in a submit)
    /// free-threaded
    void on_discard(unsigned num = 1) { _num_discarded.fetch_add(num); }

    /// to be called when a command buffer backed by this allocator
    /// is being submitted, along with the (pre-refcount-incremented) fence index that was
    /// used during the submission
    /// free-threaded
    void on_submit(unsigned num, unsigned fence_index);

    /// non-blocking reset attempt
    /// returns true if the allocator is usable afterwards
    [[nodiscard]] bool try_reset(VkDevice device);

    /// blocking reset attempt
    /// returns true if the allocator is usable afterwards
    [[nodiscard]] bool try_reset_blocking(VkDevice device);

private:
    bool is_submit_counter_up_to_date() const
    {
        // _num_in_flight is synchronized as this method is only called from the owning thread
        // the load order on the other two atomics does not matter since they monotonously increase
        // and never surpass _num_in_flight
        return _num_in_flight == _num_discarded + _num_pending_execution;
    }

    void do_reset(VkDevice device);

private:
    // non-owning
    FenceRingbuffer* _fence_ring;

    VkCommandPool _cmd_pool;
    cc::array<VkCommandBuffer> _cmd_buffers;

    /// amount of cmdbufs given out
    unsigned _num_in_flight = 0;
    /// amount of cmdbufs discarded, always less or equal to num_in_flight
    /// discarded command buffers cannot be reused, we always have to reset the entire pool
    std::atomic_uint _num_discarded = 0;
    /// amount of cmdbufs submitted, always less or equal to num_in_flight
    /// if #discard + #pending_exec == #in_flight, we can start making decisions about resetting
    std::atomic_uint _num_pending_execution = 0;

    /// the most recent fence index, -1u if none
    std::atomic_uint _latest_fence = unsigned(-1);
};

/// A bundle of single command allocators which automatically
/// circles through them and soft-resets when possible
/// Unsynchronized
class CommandAllocatorBundle
{
public:
    void initialize(VkDevice device, int num_allocators, int num_cmdlists_per_allocator, unsigned queue_family_index, FenceRingbuffer* fence_ring);
    void destroy(VkDevice device);

    /// Resets the given command list to use memory by an appropriate allocator
    /// Returns a pointer to the backing allocator node
    cmd_allocator_node* acquireMemory(VkDevice device, VkCommandBuffer& out_buffer);

private:
    void updateActiveIndex(VkDevice device);

private:
    cc::array<cmd_allocator_node> mAllocators;
    size_t mActiveAllocator = 0u;
};

class BackendVulkan;

/// The high-level allocator for Command Lists
/// Synchronized
class CommandListPool
{
public:
    // frontend-facing API (not quite, command_list can only be compiled immediately)

    [[nodiscard]] handle::command_list create(VkCommandBuffer& out_cmdlist, CommandAllocatorBundle& thread_allocator);

    /// acquire a fence to be used for command buffer submission, returns the index
    /// ONLY use the resulting index ONCE in either of the two freeOnSubmit overloads
    [[nodiscard]] unsigned acquireFence(VkFence& out_fence) { return mFenceRing.acquireFence(mDevice, out_fence); }

    /// to be called when the given command list has been submitted, alongside the fence index that was used
    /// the fence index is now consumed and must not be reused
    void freeOnSubmit(handle::command_list cl, unsigned fence_index);

    /// to be called when the given command lists have been submitted, alongside the fence index that was used
    /// the fence index is now consumed and must not be reused
    void freeOnSubmit(cc::span<handle::command_list const> cls, unsigned fence_index);

    void freeOnDiscard(handle::command_list cl);

public:
    [[nodiscard]] VkCommandBuffer getRawBuffer(handle::command_list cl) const { return mPool.get(static_cast<unsigned>(cl.index)).raw_buffer; }

    [[nodiscard]] backend::detail::incomplete_state_cache* getStateCache(handle::command_list cl)
    {
        return &mPool.get(static_cast<unsigned>(cl.index)).state_cache;
    }

public:
    void initialize(BackendVulkan& backend, int num_allocators_per_thread, int num_cmdlists_per_allocator, cc::span<CommandAllocatorBundle*> thread_allocators);
    void destroy();

private:
    struct cmd_list_node
    {
        // an allocated node is always in the following state:
        // - the command list is freshly reset using an appropriate allocator
        // - the responsible_allocator must be informed on submit or discard
        cmd_allocator_node* responsible_allocator;
        backend::detail::incomplete_state_cache state_cache;
        VkCommandBuffer raw_buffer;
    };

    // non-owning
    VkDevice mDevice;

    /// the fence ringbuffer
    FenceRingbuffer mFenceRing;

    /// the pool itself, managing handle association as well as additional
    /// bookkeeping data structures
    backend::detail::linked_pool<cmd_list_node, unsigned> mPool;

    std::mutex mMutex;
};

}