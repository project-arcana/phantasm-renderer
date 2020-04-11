#pragma once

#include <mutex>

#include <clean-core/assert.hh>

#include <phantasm-hardware-interface/fwd.hh>
#include <phantasm-hardware-interface/types.hh>

#include "circular_buffer.hh"

namespace pr
{
using gpu_epoch_t = uint64_t;

// ringbuffer for events, keeps track of GPU progress relative to CPU submissions
// CPU and GPU "epoch": steadily increasing, GPU always behind CPU
// based on acquiring events and returning them on commandlist submits
//
// internally synchronized
struct gpu_epoch_tracker
{
private:
    struct event_elem
    {
        phi::handle::event event;
        gpu_epoch_t acquired_epoch_cpu;
    };

public:
    void initialize(phi::Backend* backend, size_t max_num_events);

    void destroy();

    [[nodiscard]] phi::handle::event get_event();

    gpu_epoch_t on_event_submission(phi::handle::event event);

    /// returns the epoch that is current on the CPU
    gpu_epoch_t get_current_epoch_cpu() const { return _current_epoch_cpu; }

    /// returns the epoch that has been reached on the GPU (<= CPU)
    gpu_epoch_t get_current_epoch_gpu() const { return _current_epoch_gpu; }

private:
    std::mutex mMutex;
    phi::Backend* _backend = nullptr;
    gpu_epoch_t _current_epoch_cpu = 0;
    gpu_epoch_t _current_epoch_gpu = 0;
    circular_buffer<event_elem> _event_ring;
};
}
