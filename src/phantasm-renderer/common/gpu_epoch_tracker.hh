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
public:
    void initialize(phi::Backend* backend);
    void destroy();

    /// increments CPU epoch, signals direct queue to CPU epoch
    void increment_epoch();

    /// returns the epoch that is current on the CPU
    gpu_epoch_t get_current_epoch_cpu() const { return _current_epoch_cpu; }

    /// returns the epoch that has been reached on the GPU (<= CPU)
    gpu_epoch_t get_current_epoch_gpu() const;

private:
    phi::Backend* _backend = nullptr;
    gpu_epoch_t _current_epoch_cpu = 1; // start 1 ahead of GPU
    phi::handle::fence _fence;
};
}
