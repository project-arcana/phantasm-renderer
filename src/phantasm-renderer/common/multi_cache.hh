#pragma once

#include <atomic>
#include <mutex>

#include <clean-core/alloc_vector.hh>
#include <clean-core/alloc_array.hh>
#include <clean-core/atomic_linked_pool.hh>
#include <clean-core/map.hh>
#include <clean-core/span.hh>

#include <clean-core/allocators/synced_tlsf_allocator.hh>

#include <phantasm-renderer/common/circular_buffer.hh>
#include <phantasm-renderer/common/resource_info.hh>
#include <phantasm-renderer/fwd.hh>

#include <phantasm-renderer/resource_types.hh>

namespace pr
{
template <class KeyT>
struct ResourceCache
{
private:
    struct Bucket;
public:
    struct CachedResource
    {
        phi::handle::resource handle = phi::handle::null_resource;
        std::atomic_int32_t refcount = 0;
        gpu_epoch_t requiredGPUEpoch = 0;
        Bucket* originBucket = nullptr;
    };

    void initialize(cc::allocator* staticAlloc, uint32_t maxNumCachedResources, uint32_t expectedMaxNumKeys)
    {
        mCachedResourceIndicesMemory.reset(staticAlloc, sizeof(uint32_t) * uint32_t(1.25f * float(maxNumCachedResources)));
        mCachedResourceIndicesAllocator.initialize(mCachedResourceIndicesMemory);
        mPool.initialize(maxNumCachedResources, staticAlloc);
        mMap.reserve(expectedMaxNumKeys);
    }

    void destroy()
    {
        mMap = {};
        mPool.destroy();
        mCachedResourceIndicesAllocator.destroy();
        mCachedResourceIndicesMemory = {};
    }

    uint32_t acquireResource(KeyT const& key, gpu_epoch_t currentGPUEpoch)
    {
        auto lg = std::lock_guard(mMutex);
        Bucket& elem = accessMapBucket(key);

        for (auto i = 0u; i < elem.cachedResources.size();)
        {
            uint32_t const handle = elem.cachedResources[i];

            CC_ASSERT(mPool.is_alive(handle) && "handle unexpectedly not alive");

            CachedResource& res = mPool.get(handle);

            if (isResourceUsable(res, currentGPUEpoch))
            {
                res.refcount = 1;
                return handle;
            }

            ++i;
        }

        // no resource available, callsite will have to use insertAndAcquireResource
        return uint32_t(-1);
    }

    uint32_t insertResource(KeyT const& key, phi::handle::resource res, bool acquireImmediately)
    {
        uint32_t newHandle = mPool.acquire();
        CachedResource& newNode = mPool.get(newHandle);
        newNode.refcount = acquireImmediately ? 1 : 0;
        newNode.requiredGPUEpoch = 0;
        newNode.handle = res;

        {
            auto lg = std::lock_guard(mMutex);
            Bucket& elem = accessMapBucket(key);
            newNode.originBucket = &elem;
            elem.cachedResources.push_back(newHandle);
        }

        return acquireImmediately ? newHandle : uint32_t(-1);
    }

    // reduces the refcount on a previously acquired resource
    // given the current CPU epoch (that must be GPU-reached for the value to no longer be in flight)
    // returns true if it was freed
    bool derefResource(uint32_t handle, gpu_epoch_t currentCPUEpoch)
    {
        CachedResource& node = mPool.get(handle);
        int32_t const prevRefcount = node.refcount.fetch_add(-1);
        CC_ASSERT(prevRefcount > 0 && "Dereferenced an unreferenced resource");

        if (prevRefcount == 1)
        {
            // this was the last deref
            node.requiredGPUEpoch = currentCPUEpoch;
            return true;
        }

        return false;
    }

    // increases the refcount on a previously acquired resource
    void addrefResource(uint32_t handle)
    {
        CachedResource& node = mPool.get(handle);
        int32_t const prevRefcount = node.refcount.fetch_add(1);
        CC_ASSERT(prevRefcount > 0 && "Called addref on an unreferenced resource");
    }

    // culls non-in-flight resources from buckets that have not been acquired lately
    // returns amount of resources written to outResourcesToFree
    uint32_t cullUnusedBuckets(gpu_epoch_t currentGPUEpoch, cc::span<phi::handle::resource> outResourcesToFree, uint64_t maxGenDistance = 2)
    {
        CC_CONTRACT(outResourcesToFree.size() > 0);

        uint32_t numCulledResources = 0;
        auto lg = std::lock_guard(mMutex);
        for (auto&& [key, val] : mMap)
        {
            if (mCurrentGeneration - val.lastAccessGeneration <= maxGenDistance)
            {
                // this bucket was used recently
                continue;
            }
            cc::alloc_vector<uint32_t>& cachedResources = val.cachedResources;

            // cull the bucket
            bool isOutputFull = false;
            for (auto i = 0u; i < cachedResources.size();)
            {
                uint32_t const handle = cachedResources[i];
                CC_ASSERT(mPool.is_alive(handle) && "handle unexpectedly not alive");
                CachedResource& res = mPool.get(handle);

                if (isResourceUsable(res, currentGPUEpoch))
                {
                    outResourcesToFree[numCulledResources++] = res.handle;

                    if (numCulledResources == uint32_t(outResourcesToFree.size()))
                    {
                        isOutputFull = true;
                        break;
                    }

                    mPool.release(handle);
                    cachedResources.remove_at_unordered(i);
                    continue;
                }

                ++i;
            }

            if (isOutputFull)
            {
                break;
            }
        }

        mCurrentGeneration++;
        return numCulledResources;
    }

    // func(phi::handle::resource) called for each resource in the cache that is not in flight
    // all affected resources are removed from the cache and won't be reacquireable in the future
    template <class F>
    void cullAll(gpu_epoch_t currentGPUEpoch, F&& freeFunc)
    {
        auto lg = std::lock_guard(mMutex);
        for (auto&& [key, val] : mMap)
        {
            cc::alloc_vector<uint32_t>& cachedResources = val.cachedResources;

            for (auto i = 0u; i < cachedResources.size();)
            {
                uint32_t const handle = cachedResources[i];
                CC_ASSERT(mPool.is_alive(handle) && "handle unexpectedly not alive");
                CachedResource& res = mPool.get(handle);

                if (isResourceUsable(res, currentGPUEpoch))
                {
                    freeFunc(res.handle);
                    mPool.release(handle);
                    val.cachedResources.remove_at_unordered(i);
                    continue;
                }

                ++i;
            }
        }
    }

    // func(phi::handle::resource) called for each resource in the cache
    // usually called to clean up before system shutdown
    template <class F>
    void iterateAllResources(F&& func)
    {
        auto lg = std::lock_guard(mMutex);
        mPool.iterate_allocated_nodes([&](CachedResource& node) { func(node.handle); });
    }

    CachedResource const& getResource(uint32_t handle) const { return mPool.get(handle); }

private:
    struct Bucket
    {
        cc::alloc_vector<uint32_t> cachedResources = {}; // handles into mPool
        uint64_t lastAccessGeneration = 0;
    };

private:
    Bucket& accessMapBucket(KeyT const& key)
    {
        Bucket& elem = mMap[key]; // get or create value in map
        elem.lastAccessGeneration = mCurrentGeneration;

        if (elem.cachedResources.capacity() == 0)
        {
            // newly created
            elem.cachedResources.reset_reserve(&mCachedResourceIndicesAllocator, 4);
        }

        return elem;
    }

    bool isResourceUsable(CachedResource const& res, gpu_epoch_t currentGPUEpoch)
    {
        return res.refcount == 0 && res.requiredGPUEpoch <= currentGPUEpoch;
    }

private:
    cc::map<KeyT, Bucket, resource_info_hasher> mMap;
    cc::atomic_linked_pool<CachedResource, true> mPool;
    cc::alloc_array<std::byte> mCachedResourceIndicesMemory;
    cc::synced_tlsf_allocator<cc::spin_lock> mCachedResourceIndicesAllocator;
    uint64_t mCurrentGeneration = 0;
    std::mutex mMutex;
};

// the multi cache, key-value relation 1:N
// used for textures and buffers
// internally synchronized
template <class KeyT>
struct multi_cache
{
private:
    struct map_element;

public:
    void reserve(size_t num_elems) { _map.reserve(num_elems); }

    /// acquire a value, given the current GPU epoch (which determines resources that are no longer in flight)
    [[nodiscard]] resource acquire(KeyT const& key, gpu_epoch_t current_gpu_epoch)
    {
        auto lg = std::lock_guard(_mutex);
        map_element& elem = access_element(key);
        if (!elem.in_flight_buffer.empty())
        {
            auto& tail = elem.in_flight_buffer.get_tail();

            if (tail.required_gpu_epoch <= current_gpu_epoch)
            {
                // epoch has advanced sufficiently, pop and return
                resource res = tail.val;
                elem.in_flight_buffer.pop_tail();
                return res;
            }
        }

        // no element ready (in this case, the callsite will have to create a new object)
        return {phi::handle::null_resource};
    }

    /// free a value,
    /// given the current CPU epoch (that must be GPU-reached for the value to no longer be in flight)
    void free(resource val, KeyT const& key, gpu_epoch_t current_cpu_epoch)
    {
        auto lg = std::lock_guard(_mutex);
        map_element& elem = access_element(key);
        CC_ASSERT(!elem.in_flight_buffer.full() && "Too many cached resources of this sort are in-flight at the same time");
        elem.in_flight_buffer.enqueue({val, current_cpu_epoch});
    }

    /// conservatively frees elements that are not in flight and deemed unused
    template <class F>
    void cull(gpu_epoch_t current_gpu_epoch, F&& free_func)
    {
        (void)current_gpu_epoch;
        (void)free_func;
        auto lg = std::lock_guard(_mutex);
        ++_current_gen;
        // TODO go through a subsection of the map, and if the last gen used is old, delete old entries
    }

    /// frees all elements that are not in flight
    template <class F>
    void cull_all(gpu_epoch_t current_gpu_epoch, F&& free_func)
    {
        auto lg = std::lock_guard(_mutex);
        for (auto&& [key, val] : _map)
        {
            circular_buffer<in_flight_val>& buffer = val.in_flight_buffer;

            while (!buffer.empty() && buffer.get_tail().required_gpu_epoch <= current_gpu_epoch)
            {
                free_func(buffer.get_tail().val.handle);
                buffer.pop_tail();
            }
        }
    }

    template <class F>
    void iterate_values(F&& func)
    {
        auto lg = std::lock_guard(_mutex);
        for (auto&& [key, val] : _map)
        {
            val.in_flight_buffer.iterate_reset([&](in_flight_val const& if_val) { func(if_val.val.handle); });
        }
    }

private:
    map_element& access_element(KeyT const& key)
    {
        map_element& elem = _map[key];
        elem.latest_gen = _current_gen;

        if (elem.in_flight_buffer.capacity() == 0)
        {
            // newly created
            elem.in_flight_buffer = circular_buffer<in_flight_val>(32);
        }

        return elem;
    }

private:
    struct in_flight_val
    {
        resource val;
        gpu_epoch_t required_gpu_epoch;
    };

    struct map_element
    {
        circular_buffer<in_flight_val> in_flight_buffer;
        uint64_t latest_gen = 0;
    };

    uint64_t _current_gen = 0;
    cc::map<KeyT, map_element, resource_info_hasher> _map;
    std::mutex _mutex;
};

}
