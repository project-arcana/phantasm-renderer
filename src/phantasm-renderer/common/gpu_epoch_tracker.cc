#include "gpu_epoch_tracker.hh"

#include <phantasm-hardware-interface/Backend.hh>

void pr::gpu_epoch_tracker::initialize(phi::Backend* backend, size_t max_num_events)
{
    _backend = backend;
    _event_ring = circular_buffer<event_elem>(max_num_events);
}

void pr::gpu_epoch_tracker::destroy()
{
    auto const lg = std::lock_guard(mMutex);
    while (!_event_ring.empty())
    {
        _backend->free(cc::span{_event_ring.get_tail().event});
        _event_ring.pop_tail();
    }
}

phi::handle::event pr::gpu_epoch_tracker::get_event()
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

void pr::gpu_epoch_tracker::on_event_submission(phi::handle::event event)
{
    auto const lg = std::lock_guard(mMutex);
    CC_ASSERT(!_event_ring.full() && "event ring full");
    ++_current_epoch_cpu;
    _event_ring.enqueue({event, _current_epoch_cpu});
}
