#pragma once

#include <cstddef>
#include <type_traits>

#include <clean-core/move.hh>
#include <clean-core/utility.hh>

namespace pr
{
template <class T>
class circular_buffer
{
public:
    explicit circular_buffer() = default;
    explicit circular_buffer(size_t size) : _buffer(new T[size]), _num_elements(size), _is_full(false) {}

    circular_buffer(circular_buffer const& rhs) = delete;
    circular_buffer& operator=(circular_buffer const& rhs) = delete;

    circular_buffer(circular_buffer&& rhs) noexcept
    {
        _buffer = rhs._buffer;
        rhs._buffer = nullptr;
        _num_elements = rhs._num_elements;
        _head = rhs._head;
        _tail = rhs._tail;
        _is_full = rhs._is_full;
    }

    circular_buffer& operator=(circular_buffer&& rhs) noexcept
    {
        if (this != &rhs)
        {
            if (_buffer)
                delete[] _buffer;
            _buffer = rhs._buffer;
            rhs._buffer = nullptr;
            _num_elements = rhs._num_elements;
            _head = rhs._head;
            _tail = rhs._tail;
            _is_full = rhs._is_full;
        }
        return *this;
    }


    ~circular_buffer()
    {
        if (_buffer)
            delete[] _buffer;
    }

    void enqueue(T const& item)
    {
        CC_ASSERT(!full());

        _buffer[_head] = item;

        if (_is_full)
        {
            _tail = cc::wrapped_increment(_tail, _num_elements);
        }

        _head = cc::wrapped_increment(_head, _num_elements);
        _is_full = _head == _tail;
    }

    void enqueue(T&& item)
    {
        CC_ASSERT(!full());

        _buffer[_head] = cc::move(item);

        if (_is_full)
        {
            _tail = cc::wrapped_increment(_tail, _num_elements);
        }

        _head = cc::wrapped_increment(_head, _num_elements);
        _is_full = _head == _tail;
    }

    [[nodiscard]] T const& get_tail() const { return _buffer[_tail]; }
    [[nodiscard]] T& get_tail() { return _buffer[_tail]; }

    void pop_tail()
    {
        CC_ASSERT(!empty() && "pop_tail on an empty queue");
        _is_full = false;
        _tail = cc::wrapped_increment(_tail, _num_elements);
    }

    void reset()
    {
        _head = _tail;
        _is_full = false;
    }

    bool empty() const
    {
        // if head and tail are equal, we are empty
        return (!_is_full && (_head == _tail));
    }

    bool full() const
    {
        // If tail is ahead the head by 1, we are full
        return _is_full;
    }

    size_t capacity() const { return _num_elements; }

    size_t size() const
    {
        size_t size = _num_elements;

        if (!_is_full)
        {
            if (_head >= _tail)
            {
                size = _head - _tail;
            }
            else
            {
                size = _num_elements + _head - _tail;
            }
        }

        return size;
    }

private:
    T* _buffer = nullptr;
    size_t _num_elements = 0;
    size_t _head = 0;
    size_t _tail = 0;
    bool _is_full = true;
};
}
