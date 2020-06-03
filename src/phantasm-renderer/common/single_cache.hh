#pragma once

#include <mutex>

#include <clean-core/map.hh>
#include <clean-core/typedefs.hh>

#include <rich-log/log.hh>

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/fwd.hh>

namespace pr
{
// the single cache, key-value relation 1:1
// used for PSOs, shader views
// internally synchronized
template <class ValT>
struct single_cache
{
private:
    static constexpr ValT invalid_val = ValT{phi::handle::null_handle_index};
    struct map_element;

public:
    void reserve(size_t num_elems) { _map.reserve(num_elems); }

    [[nodiscard]] ValT acquire(cc::hash_t key)
    {
        auto lg = std::lock_guard(_mutex);
        map_element& elem = _map[key];
        if (elem.val != invalid_val)
        {
            ++elem.num_references;
        }

        return elem.val;
    }

    void insert(ValT val, cc::hash_t key)
    {
        auto lg = std::lock_guard(_mutex);
        CC_ASSERT(val != invalid_val && "[single_cache] invalid value inserted");
        map_element& elem = _map[key];
        elem.val = val;
        elem.num_references = 1;
        elem.required_gpu_epoch = 0;
    }

    void free(cc::hash_t key, gpu_epoch_t current_cpu_epoch)
    {
        auto lg = std::lock_guard(_mutex);
        map_element& elem = _map[key];
        CC_ASSERT(elem.val != invalid_val && "[single_cache] freed an element not previously inserted");
        CC_ASSERT(elem.num_references > 0 && "[single_cache] freed an element not previously acquired");
        --elem.num_references;
        elem.required_gpu_epoch = current_cpu_epoch;
    }

    template <class F>
    void cull_all(gpu_epoch_t current_gpu_epoch, F&& destroy_func)
    {
        auto lg = std::lock_guard(_mutex);
        auto f_can_cull = [&](map_element const& elem) { return elem.num_references == 0 && elem.required_gpu_epoch <= current_gpu_epoch; };

        for (auto&& [key, elem] : _map)
        {
            if (f_can_cull(elem))
            {
                destroy_func(elem.val);
                CC_RUNTIME_ASSERT(false && "cc::map remove unimplemented and required here");
            }
        }
    }

    template <class F>
    void iterate_values(F&& func)
    {
        auto lg = std::lock_guard(_mutex);
        for (auto const& [key, val] : _map)
        {
            func(val.val);
        }
    }

private:
    struct map_element
    {
        ValT val = invalid_val;
        uint32_t num_references = 0;        ///< the amount of CPU-side references to this element
        gpu_epoch_t required_gpu_epoch = 0; ///< CPU epoch when this element was last freed
    };

    cc::map<cc::hash_t, map_element> _map;
    std::mutex _mutex;
};
}
