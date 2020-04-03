#pragma once

#include <mutex>

#include <clean-core/assert.hh>

#include <phantasm-hardware-interface/Backend.hh>

#include "circular_buffer.hh"

namespace pr
{
struct gpu_epoch_tracker
{
    void initialize(phi::Backend* backend, size_t max_num_events)
    {
        _backend = backend;
        _event_ring = circular_buffer<event_elem>(max_num_events);
    }

    void destroy()
    {
        auto const lg = std::lock_guard(mMutex);
        while (!_event_ring.empty())
        {
            _backend->free(cc::span{_event_ring.get_tail().event});
            _event_ring.pop_tail();
        }
    }

    [[nodiscard]] phi::handle::event get_event()
    {
        {
            auto const lg = std::lock_guard(mMutex);
            if (!_event_ring.empty())
            {
                // try clearing the oldest event in the buffer
                auto const tail = _event_ring.get_tail();
                if (_backend->clearEvent(tail.event))
                {
                    // cleared, this event was reached on GPU, advance gpu epoch
                    _current_epoch_gpu = tail.acquired_epoch_cpu;
                    // pop and return
                    _event_ring.pop_tail();
                    return tail.event;
                }
                else
                {
                    // oldest event not yet reached, fall through to new event creation
                }
            }
        }

        // create a new event
        return _backend->createEvent();
    }

    void on_event_submission(phi::handle::event event)
    {
        auto const lg = std::lock_guard(mMutex);
        CC_ASSERT(!_event_ring.full() && "event ring full");
        ++_current_epoch_cpu;
        _event_ring.enqueue({event, _current_epoch_cpu});
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

    std::mutex mMutex;
    phi::Backend* _backend = nullptr;
    uint64_t _current_epoch_cpu = 0;
    uint64_t _current_epoch_gpu = 0;
    circular_buffer<event_elem> _event_ring;
};
}
