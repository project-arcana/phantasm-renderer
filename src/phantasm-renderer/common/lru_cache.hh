#pragma once

#include <unordered_map>

#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/types.hh>

namespace pr
{
template <class KeyT, class ValT>
struct lru_cache
{
    [[nodiscard]] ValT acquire(KeyT const& key)
    {
        map_element& elem = _map[key];
        elem.latest_gen = _current_gen;

        if (!elem.free_vals.empty())
        {
            auto const res = elem.free_vals.back();
            elem.free_vals.pop_back();
            return res;
        }
        else if ()
        {

        }

        // if (has_free_val)
        //      return free
        // elif (has_pending_vals)
        //      check pending
        //      return if any is free
        //
        // create & return new

        // result state is acquired
    }

    void free(ValT val, phi::handle::event dependency)
    {
        // assert(val.state == acquired)
        // change state to pending
        // add dependency

        // free things (?)
    }

    void cull() { ++_current_gen; }

private:
    struct in_flight_val
    {
        ValT val;
        phi::handle::event dependency;
    };

    struct map_element
    {
        cc::vector<ValT> free_vals;
        cc::vector<in_flight_val> in_flight_vals;
        uint64_t latest_gen = 0;
    };

    uint64_t _current_gen = 0;
    std::unordered_map<KeyT, map_element> _map;
};

}
