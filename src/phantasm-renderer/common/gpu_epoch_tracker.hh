#pragma once

#include <phantasm-hardware-interface/fwd.hh>
#include <phantasm-hardware-interface/types.hh>

namespace pr
{
using gpu_epoch_t = uint64_t;

// keeps track of GPU progress relative to CPU submissions
// CPU and GPU "epoch": steadily increasing, GPU always behind CPU
//
// internally synchronized
struct gpu_epoch_tracker
{
    void initialize(phi::Backend* backend);
    void destroy(phi::Backend* backend);

    /// returns the epoch that is current on the CPU
    gpu_epoch_t get_current_epoch_cpu() const { return _current_epoch_cpu; }

    /// returns the epoch that has been reached on the GPU (<= CPU)
    gpu_epoch_t get_current_epoch_gpu(phi::Backend* backend) const;

    gpu_epoch_t _current_epoch_cpu = 1; // start 1 ahead of GPU
    gpu_epoch_t _cached_epoch_gpu = 0;  // cached, always <= real GPU
    phi::handle::fence _fence;
};
}
