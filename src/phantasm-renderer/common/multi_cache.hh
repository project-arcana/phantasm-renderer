#pragma once

#include <unordered_map>

#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/circular_buffer.hh>
#include <phantasm-renderer/common/resource_info.hh>

#include <phantasm-renderer/resource_types.hh>

namespace pr
{
template <class KeyT>
struct multi_cache
{
private:
    struct map_element;

public:
    void reserve(size_t num_elems) { _map.reserve(num_elems); }

    /// acquire a value, given the current GPU epoch (which determines resources that are no longer in flight)
    [[nodiscard]] pr::resource acquire(KeyT const& key, uint64_t current_gpu_epoch)
    {
        map_element& elem = access_element(key);
        if (!elem.in_flight_buffer.empty())
        {
            // always per value
            auto const tail = elem.in_flight_buffer.get_tail();

            if (tail.required_gpu_epoch <= current_gpu_epoch)
            {
                // event is ready, pop and return
                auto const res = tail.val;
                elem.in_flight_buffer.pop_tail();
                return res;
            }
        }

        // no element ready (in this case, the callsite will have to create a new object)
        return {};
    }

    /// free a value,
    /// given the current CPU epoch (that must be GPU-reached for the value to no longer be in flight)
    void free(pr::resource val, KeyT const& key, uint64_t current_cpu_epoch)
    {
        map_element& elem = access_element(key);
        CC_ASSERT(!elem.in_flight_buffer.full());
        elem.in_flight_buffer.enqueue({val, current_cpu_epoch});
    }

    void cull()
    {
        ++_current_gen;
        // todo: go through a subsection of the map, and if the last gen used is old, delete old entries
    }

    void free_all(phi::Backend* backend)
    {
        for (auto& [key, elem] : _map)
        {
            while (!elem.in_flight_buffer.empty())
            {
                backend->free(elem.in_flight_buffer.get_tail().val._handle);
                elem.in_flight_buffer.pop_tail();
            }
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
        pr::resource val;
        uint64_t required_gpu_epoch;
    };

    struct map_element
    {
        circular_buffer<in_flight_val> in_flight_buffer;
        uint64_t latest_gen = 0;
    };

    uint64_t _current_gen = 0;
    std::unordered_map<KeyT, map_element, resource_info_hasher> _map;
};

}