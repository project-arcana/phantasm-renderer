#pragma once

#include <clean-core/map.hh>

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/resource_info.hh>

namespace pr
{
template <class KeyT, class ValT, class HasherT>
struct single_cache
{
private:
    struct map_element;

public:
    void reserve(size_t num_elems) { _map.reserve(num_elems); }

private:
    map_element& access_element(KeyT const& key)
    {
        map_element& elem = _map[key];
        elem.latest_gen = _current_gen;
        return elem;
    }

private:
    struct map_element
    {
        ValT val;
        uint32_t num_references = 0;     ///< the amount of CPU-side references to this element
        uint64_t latest_gen = 0;         ///< generation when this element was last acquired
        uint64_t required_gpu_epoch = 0; ///< CPU epoch when this element was last freed
    };

    uint64_t _current_gen = 0;
    cc::map<KeyT, map_element, HasherT> _map;
};

// template <class KeyT>
// struct multi_cache
//{
// private:
//    struct map_element;

// public:
//    void reserve(size_t num_elems) { _map.reserve(num_elems); }

//    /// acquire a value, given the current GPU epoch (which determines resources that are no longer in flight)
//    [[nodiscard]] int acquire(KeyT const& key, uint64_t current_gpu_epoch)
//    {
//        map_element& elem = access_element(key);
//        elem.last_access_gpu_epoch = current_gpu_epoch;
//        return elem.phi_handle;
//    }

//    /// free a value,
//    /// given the current CPU epoch (that must be GPU-reached for the value to no longer be in flight)
//    void free(pr::resource&& val, KeyT const& key, uint64_t current_cpu_epoch)
//    {
//        map_element& elem = access_element(key);
//        CC_ASSERT(!elem.in_flight_buffer.full());
//        elem.in_flight_buffer.enqueue({cc::move(val), current_cpu_epoch});
//    }

//    void cull()
//    {
//        ++_current_gen;
//        // todo: go through a subsection of the map, and if the last gen used is old, delete old entries
//    }

// private:
//    map_element& access_element(KeyT const& key)
//    {
//        map_element& elem = _map[key];
//        elem.latest_gen = _current_gen;
//        return elem;
//    }

// private:
//    struct map_element
//    {
//        int phi_handle = phi::handle::null_handle_index; /// the phi::handle value
//        uint64_t last_access_gpu_epoch = 0;              /// the GPU epoch during the last (acquire/free) access
//        uint64_t last_access_gen = 0;                    /// the cache generation during the last (acquire/free) access
//    };

//    uint64_t _current_gen = 0;
//    cc::map<KeyT, map_element, resource_info_hasher> _map;
//};

}
