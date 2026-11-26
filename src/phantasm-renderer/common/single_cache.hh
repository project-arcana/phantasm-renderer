#pragma once

#include <mutex>

#include <clean-core/map.hh>
#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/handles.hh>

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
    static constexpr ValT invalid_val = ValT{phi::handle::null_handle_value};
    struct map_element;

public:
    void reserve(size_t num_elems) { _map.reserve(num_elems); }

    [[nodiscard]] ValT acquire(uint64_t key)
    {
        auto lg = std::lock_guard(_mutex);
        map_element& elem = _map[key];
        if (elem.val != invalid_val)
        {
            ++elem.num_references;
        }

        return elem.val;
    }

    // returns true if the element was inserted
    // returns false and writes to p_out_preexisting_val if the element already existed
    bool try_insert(ValT val, uint64_t key, ValT* p_out_preexisting_val)
    {
        auto lg = std::lock_guard(_mutex);
        CC_ASSERT(val != invalid_val && "[single_cache] invalid value inserted");
        map_element& elem = _map[key];
        if (elem.val != invalid_val)
        {
            *p_out_preexisting_val = elem.val;
            ++elem.num_references;
            return false;
        }

        elem.val = val;
        elem.num_references = 1;
        elem.required_gpu_epoch = 0;
        return true;
    }

    void free(uint64_t key, gpu_epoch_t current_cpu_epoch)
    {
        auto lg = std::lock_guard(_mutex);
        map_element& elem = _map[key];
        CC_ASSERT(elem.val != invalid_val && "[single_cache] freed an element not previously inserted");
        CC_ASSERT(elem.num_references > 0 && "[single_cache] freed an element not previously acquired");
        --elem.num_references;
        elem.required_gpu_epoch = current_cpu_epoch;
    }

    /// destroys all elements that are not currently referenced (CPU) or in flight (GPU)
    template <class F>
    void cull_all(gpu_epoch_t current_gpu_epoch, F&& destroy_func)
    {
        auto lg = std::lock_guard(_mutex);
        auto f_can_cull = [&](map_element const& elem) -> bool { return elem.num_references == 0 && elem.required_gpu_epoch <= current_gpu_epoch; };

        cc::vector<uint64_t> keys_to_remove;
        keys_to_remove.reserve(_map.size());

        for (auto&& [key, elem] : _map)
        {
            if (f_can_cull(elem))
            {
                destroy_func(elem.val);
                keys_to_remove.push_back(key);
            }
        }

        for (auto key : keys_to_remove)
            _map.remove_key(key);
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

    cc::map<uint64_t, map_element> _map;
    std::mutex _mutex;
};
}
