#pragma once

#include <clean-core/array.hh>
#include <clean-core/new.hh>

namespace pr::backend::detail
{
template <class T, class IdxT = size_t>
struct linked_pool
{
    static_assert(sizeof(T) >= sizeof(T*), "linked_pool element type must be large enough to accomodate a pointer");
    using index_t = IdxT;

    void initialize(size_t size)
    {
        if (size == 0)
            return;

        CC_ASSERT(size < index_t(-1));

        _pool = cc::array<T>::uninitialized(size);

        // initialize linked list
        for (auto i = 0u; i < _pool.size() - 1; ++i)
        {
            T* node_ptr = &_pool[i];
            new (cc::placement_new, node_ptr) T*(&_pool[i + 1]);
        }

        // initialize linked list tail
        {
            T* tail_ptr = &_pool[_pool.size() - 1];
            new (cc::placement_new, tail_ptr) T*(nullptr);
        }

        _first_free_node = &_pool[0];
    }

    [[nodiscard]] index_t acquire()
    {
        CC_ASSERT(_first_free_node != nullptr && "linked_pool full");

        T* const acquired_node = _first_free_node;
        // read the in-place next pointer of this node
        _first_free_node = *reinterpret_cast<T**>(acquired_node);
        // call the constructor
        new (cc::placement_new, acquired_node) T();

        return static_cast<index_t>(acquired_node - _pool.begin());
    }

    void release(index_t index)
    {
        T* const released_node = &_pool[static_cast<size_t>(index)];
        // call the destructor
        released_node->~T();
        // write the in-place next pointer of this node
        new (cc::placement_new, released_node) T*(_first_free_node);

        _first_free_node = released_node;
    }

    T& get(index_t index) { return _pool[static_cast<size_t>(index)]; }
    T const& get(index_t index) const { return _pool[static_cast<size_t>(index)]; }

private:
    T* _first_free_node = nullptr;
    cc::array<T> _pool;
};

}
