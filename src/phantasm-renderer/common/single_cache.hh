#pragma once

#include <unordered_map>

namespace pr
{
template <class KeyT, class ValT, class HasherT>
struct single_cache
{
public:
private:
    struct map_element
    {
        ValT val;
        uint64_t latest_gen;
        uint64_t required_gpu_epoch;
    };

    uint64_t _current_gen = 0;
    std::unordered_map<KeyT, map_element, HasherT> _map;
};

}
