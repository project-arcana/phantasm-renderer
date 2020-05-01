#pragma once

#include <mutex>

#include <clean-core/map.hh>

#include <phantasm-renderer/common/circular_buffer.hh>
#include <phantasm-renderer/common/resource_info.hh>
#include <phantasm-renderer/fwd.hh>

#include <phantasm-renderer/resource_types.hh>

namespace pr
{
// the multi cache, key-value relation 1:N
// used for render targets, textures, buffers
// internally synchronized
template <class KeyT>
struct multi_cache
{
private:
    struct map_element;

public:
    void reserve(size_t num_elems) { _map.reserve(num_elems); }

    /// acquire a value, given the current GPU epoch (which determines resources that are no longer in flight)
    [[nodiscard]] raw_resource acquire(KeyT const& key, gpu_epoch_t current_gpu_epoch)
    {
        auto lg = std::lock_guard(_mutex);
        map_element& elem = access_element(key);
        if (!elem.in_flight_buffer.empty())
        {
            auto& tail = elem.in_flight_buffer.get_tail();

            if (tail.required_gpu_epoch <= current_gpu_epoch)
            {
                // epoch has advanced sufficiently, pop and return
                raw_resource res = tail.val;
                elem.in_flight_buffer.pop_tail();
                return res;
            }
        }

        // no element ready (in this case, the callsite will have to create a new object)
        return {phi::handle::null_resource, 0};
    }

    /// free a value,
    /// given the current CPU epoch (that must be GPU-reached for the value to no longer be in flight)
    void free(raw_resource val, KeyT const& key, gpu_epoch_t current_cpu_epoch)
    {
        auto lg = std::lock_guard(_mutex);
        map_element& elem = access_element(key);
        CC_ASSERT(!elem.in_flight_buffer.full());
        elem.in_flight_buffer.enqueue({val, current_cpu_epoch});
    }

    /// conservatively frees elements that are not in flight and deemed unused
    template <class F>
    void cull(gpu_epoch_t current_gpu_epoch, F&& free_func)
    {
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
            elem.in_flight_buffer = circular_buffer<in_flight_val>(512);
        }

        return elem;
    }

private:
    struct in_flight_val
    {
        raw_resource val;
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
