#pragma once

#include <clean-core/map.hh>

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/state_info.hh>

namespace pr
{
// the single cache, key-value relation 1:1
// used for PSOs, shader views
template <class KeyT, class ValT>
struct single_cache
{
private:
    static constexpr ValT invalid_val = ValT{phi::handle::null_handle_index};
    struct map_element;

public:
    void reserve(size_t num_elems) { _map.reserve(num_elems); }

    [[nodiscard]] ValT acquire(KeyT const& key)
    {
        map_element& elem = _map[key];
        if (elem.val != invalid_val)
        {
            ++elem.num_references;
        }

        return elem.val;
    }

    void insert(ValT val, KeyT const& key)
    {
        CC_ASSERT(val != invalid_val && "[single_cache] invalid value inserted");
        map_element& elem = _map[key];
        elem.val = val;
        elem.num_references = 1;
        elem.required_gpu_epoch = 0;
    }

    void free(KeyT const& key, uint64_t current_cpu_epoch)
    {
        map_element& elem = _map[key];
        CC_ASSERT(elem.val != invalid_val && "[single_cache] freed an element not previously inserted");
        CC_ASSERT(elem.num_references > 0 && "[single_cache] freed an element not previously acquired");
        --elem.num_references;
        elem.required_gpu_epoch = current_cpu_epoch;
    }

    template <class F>
    void cull(uint64_t current_gpu_epoch, F&& destroy_func)
    {
        auto const f_can_cull = [&](map_element const& elem) { return elem.num_references == 0 && elem.required_gpu_epoch <= current_gpu_epoch; };

        // TODO: go through section of the map and destroy element based on condition above
    }

private:
    struct map_element
    {
        ValT val = invalid_val;
        uint32_t num_references = 0;     ///< the amount of CPU-side references to this element
        uint64_t required_gpu_epoch = 0; ///< CPU epoch when this element was last freed
    };

    cc::map<KeyT, map_element, state_info_hasher> _map;
};
}
