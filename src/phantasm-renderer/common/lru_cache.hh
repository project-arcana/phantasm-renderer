#pragma once

#include <unordered_map>

#include <clean-core/utility.hh>
#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/Backend.hh>
#include <phantasm-hardware-interface/types.hh>

namespace pr
{
template <class T>
class circular_buffer
{
    static_assert(std::is_trivial_v<T>, "no lifetime management support");

public:
    explicit circular_buffer() = default;
    explicit circular_buffer(size_t size) : _buffer(new T[size]), _num_elements(size) {}

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
        _buffer[_head] = item;

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
    bool _is_full = false;
};

struct gpu_epoch_tracker
{
    void initialize(phi::Backend* backend, size_t max_num_events)
    {
        _backend = backend;
        _event_ring = circular_buffer<event_elem>(max_num_events);
    }

    void destroy()
    {
        while (!_event_ring.empty())
        {
            _backend->free(cc::span{_event_ring.get_tail().event});
            _event_ring.pop_tail();
        }
    }

    [[nodiscard]] phi::handle::event get_event()
    {
        // always advance cpu epoch
        ++_current_epoch_cpu;

        if (!_event_ring.empty())
        {
            // try clearing the oldest event in the buffer
            auto const tail = _event_ring.get_tail();
            if (_backend->tryUnsetEvent(tail.event))
            {
                // cleared, this event was reached on GPU, advance gpu epoch
                _current_epoch_gpu = tail.acquired_epoch_cpu;
                // pop and re-enqueue with the current CPU epoch
                _event_ring.pop_tail();
                _event_ring.enqueue({tail.event, _current_epoch_cpu});
                return tail.event;
            }
            else
            {
                // oldest event not yet reached, fall through to new event creation
            }
        }

        CC_ASSERT(!_event_ring.full() && "event ring full");

        // create a new event
        auto const new_event = _backend->createEvent();
        _event_ring.enqueue({new_event, _current_epoch_cpu});
        return new_event;
    }

    /// returns the epoch that is current on the CPU
    uint64_t get_current_epoch_cpu() const { return _current_epoch_cpu; }

    /// returns the epoch that has been reached on the GPU (<= CPU)
    uint64_t get_current_epoch_gpu() const { return _current_epoch_gpu; }

private:
    struct event_elem
    {
        phi::handle::event event;
        uint64_t acquired_epoch_cpu;
    };

    phi::Backend* _backend = nullptr;
    uint64_t _current_epoch_cpu = 0;
    uint64_t _current_epoch_gpu = 0;
    circular_buffer<event_elem> _event_ring;
};

template <class KeyT, class ValT>
struct lru_cache
{
    /// acquire a value, given the current GPU epoch (which determines resources that are no longer in flight)
    [[nodiscard]] ValT acquire(KeyT const& key, uint64_t current_gpu_epoch)
    {
        map_element& elem = _map[key];
        elem.latest_gen = _current_gen;

        if (!elem.in_flight_buffer.empty())
        {
            // always per value
            auto const tail = elem.in_flight_buffer.get_tail();

            if (tail.required_gpu_epoch <= current_gpu_epoch)
            {
                // event is ready, pop and return
                elem.in_flight_buffer.pop_tail();
                return tail.val;
            }
        }

        // no element ready (in this case, the callsite will simply have to create a new object)
        return {};
    }

    /// free a value (or insert it for the first time),
    /// given the current CPU epoch (that must be GPU-reached for the value to no longer be in flight)
    void free_or_insert(ValT val, uint64_t current_cpu_epoch)
    {
        map_element& elem = _map[key];
        elem.latest_gen = _current_gen;
    }

    void cull()
    {
        ++_current_gen;
        // todo: go through a subsection of the map, and if the last gen used is old, delete old entries
    }

private:
    struct in_flight_val
    {
        ValT val;
        uint64_t required_gpu_epoch;
    };

    struct map_element
    {
        circular_buffer<in_flight_val> in_flight_buffer;
        uint64_t latest_gen = 0;
    };

    uint64_t _current_gen = 0;
    std::unordered_map<KeyT, map_element> _map;
};

}
