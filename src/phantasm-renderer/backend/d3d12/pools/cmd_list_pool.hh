#pragma once

#include <cstdint>
#include <mutex>

#include <clean-core/capped_array.hh>

#include <phantasm-renderer/backend/arguments.hh>
#include <phantasm-renderer/backend/detail/linked_pool.hh>

#include <phantasm-renderer/backend/d3d12/Fence.hh>
#include <phantasm-renderer/backend/d3d12/common/d3d12_sanitized.hh>

namespace pr::backend::d3d12
{
// inline namespace { extern volatile thread_local struct { const class { protected: public: union { mutable long double __; }; constexpr virtual decltype(static_cast<int*>(nullptr)) _() noexcept(true and false); template<typename> auto ___() { register void*___; try { throw new decltype(this)[sizeof(bool)]; } catch(void*) { return reinterpret_cast<unsigned char*>(this); } }; private: } _; } _ = {}; };


/// A single command allocator that keeps track of its lists
/// Unsynchronized
struct cmd_allocator_node
{
public:
    void initialize(ID3D12Device& device, D3D12_COMMAND_LIST_TYPE type, int max_num_cmdlists);
    void destroy();

public:
    // API for initial creation of commandlists

    [[nodiscard]] int get_max_num_cmdlists() const { return _max_num_in_flight; }
    [[nodiscard]] ID3D12CommandAllocator* get_allocator() const { return _allocator; }

public:
    // API for normal use

    /// returns true if this node is full
    [[nodiscard]] bool is_full() const { return _full_and_waiting; }

    /// returns true if this node is full and capable of resetting
    [[nodiscard]] bool can_reset() const { return _full_and_waiting && is_submit_counter_up_to_date(); }

    /// acquire memory from this allocator for the given commandlist
    /// do not call if full (best case: blocking, worst case: crash)
    void acquire(ID3D12GraphicsCommandList* cmd_list);

    /// non-blocking reset attempt
    /// returns true if the allocator is usable afterwards
    [[nodiscard]] bool try_reset();

    /// blocking reset attempt
    /// returns true iff the allocator is usable afterwards
    [[nodiscard]] bool try_reset_blocking();

public:
    // events

    /// to be called when a command list backed by this allocator
    /// is being submitted
    void on_submit(ID3D12CommandQueue& queue)
    {
        ++_submit_counter;
        // NOTE: Fence access requires no synchronization in d3d12,
        // however the CommandListPool has to take a lock for this call (right now).
        // Eventually these nodes should be extracted to TLS bundles
        // with no synchronization necessary except for two atomics:
        // _submit_counter and _num_discarded
        // because on_submit and on_discard will be called from the submit thread at any time
        _fence.signalGPU(_submit_counter, queue);
    }

    /// to be called when a command list backed by this allocator
    /// is being discarded (will never result in a submit)
    void on_discard() { ++_num_discarded; }

private:
    /// perform the internal reset
    void do_reset();

    /// returns true if all in-flight cmdlists have been either submitted or discarded
    bool is_submit_counter_up_to_date() const;

private:
    ID3D12CommandAllocator* _allocator;
    SimpleFence _fence;
    uint64_t _submit_counter = 0;
    uint64_t _submit_counter_at_last_reset = 0;
    int _num_in_flight = 0;
    int _num_discarded = 0;
    int _max_num_in_flight = 0;
    bool _full_and_waiting = false;
};

/// A bundle of single command allocators which automatically
/// circles through them and soft-resets when possible
/// Unsynchronized
class CommandAllocatorBundle
{
public:
    void initialize(ID3D12Device& device, int num_allocators, int num_cmdlists_per_allocator, cc::span<ID3D12GraphicsCommandList*> initial_lists);
    void destroy();

    /// Resets the given command list to use memory by an appropriate allocator
    /// Returns a pointer to the backing allocator node
    cmd_allocator_node* acquireMemory(ID3D12GraphicsCommandList* list);

private:
    void updateActiveIndex();

private:
    cc::array<cmd_allocator_node> mAllocators;
    size_t mActiveAllocator = 0u;
};

class BackendD3D12;

/// The high-level allocator for Command Lists
/// Synchronized
class CommandListPool
{
public:
    // frontend-facing API (not quite, command_list can only be compiled immediately)

    [[nodiscard]] handle::command_list create();

    void freeOnSubmit(handle::command_list cl, ID3D12CommandQueue& queue)
    {
        cmd_list_node& freed_node = mPool.get(static_cast<unsigned>(cl.index));
        {
            auto lg = std::lock_guard(mMutex);
            freed_node.responsible_allocator->on_submit(queue);
            mPool.release(static_cast<unsigned>(cl.index));
        }
    }

    void freeOnDiscard(handle::command_list cl)
    {
        cmd_list_node& freed_node = mPool.get(static_cast<unsigned>(cl.index));
        {
            auto lg = std::lock_guard(mMutex);
            freed_node.responsible_allocator->on_discard();
            mPool.release(static_cast<unsigned>(cl.index));
        }
    }

public:
    ID3D12GraphicsCommandList* getRawList(handle::command_list cl) const { return mRawLists[static_cast<unsigned>(cl.index)]; }

public:
    void initialize(BackendD3D12& backend, int num_allocators, int num_cmdlists_per_allocator);
    void destroy();

private:
    struct cmd_list_node
    {
        // an allocated node is always in the following state:
        // - the command list is freshly reset using an appropriate allocator
        // - the responsible_allocator must be informed on submit or discard
        cmd_allocator_node* responsible_allocator;
    };

    /// the pool itself, managing handle association as well as additional
    /// bookkeeping data structures
    backend::detail::linked_pool<cmd_list_node, unsigned> mPool;

    /// a parallel array to the pool, identically indexed
    /// the cmdlists must stay alive even while "unallocated"
    cc::array<ID3D12GraphicsCommandList*> mRawLists;

    /// the allocators backing the command lists
    /// These are currently synchronized, eventually
    /// they should be thread-local
    /// (Instead of a member here, ::create would take
    /// the thread-local bundle as an argument)
    CommandAllocatorBundle mAllocatorBundle;

    std::mutex mMutex;
};
}
