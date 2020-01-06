#pragma once

#include <atomic>

#include <clean-core/array.hh>

namespace pr::backend::detail
{
template <class KeyT, class ValT, class Functor>
struct spmc_cache
{
public:
    spmc_cache(size_t size) : _buffer(size)
    {
        for (auto& node : _buffer)
            node.is_filled.store(false);
    }

    /// look up a key in the cache, out_val is only written to if found
    [[nodiscard]] bool look_up(KeyT const& key, ValT& out_val)
    {
        size_t const key_hash = _functor.hash(key);
        for (auto offset = 0u; offset < _buffer.size(); ++offset)
        {
            auto const& node = _buffer[(key_hash + offset) % _buffer.size()];

            if (node.is_filled.load(std::memory_order_acquire) == false)
            {
                return false;
            }
            else if (_functor.compare(key, node.key))
            {
                out_val = node.value;
                return true;
            }
        }
        return false;
    }

    void insert(KeyT const& key, ValT const& value)
    {
        size_t const key_hash = _functor.hash(key);
        for (auto offset = 0u; offset < _buffer.size(); ++offset)
        {
            auto& node = _buffer[(key_hash + offset) % _buffer.size()];

            if (node.is_filled.load(std::memory_order_acquire) == false)
            {
                // found empty node
                node.key = key;
                node.value = value;
                node.is_filled.store(std::memory_order_release);
                return;
            }
            else if (_functor.compare(key, node.key))
            {
                // nonempty node, which already contains this key, do nothing
                return;
            }
        }

        CC_ASSERT(false && "spmc_cache full");
    }

private:
    struct node_t
    {
        KeyT key;
        ValT value;
        std::atomic_bool is_filled;
    };

    cc::array<node_t> _buffer;
    Functor _functor;
};


}
