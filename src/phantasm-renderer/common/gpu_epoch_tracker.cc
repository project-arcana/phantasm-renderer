#include "gpu_epoch_tracker.hh"

#include <clean-core/assert.hh>

#include <phantasm-hardware-interface/Backend.hh>

void pr::gpu_epoch_tracker::initialize(phi::Backend* backend)
{
    _backend = backend;
    _fence = backend->createFence();
    CC_ASSERT(backend->getFenceValue(_fence) == 0 && "invalid fence value on init");
}

void pr::gpu_epoch_tracker::destroy() { _backend->free(cc::span{_fence}); }

void pr::gpu_epoch_tracker::increment_epoch()
{
    _backend->signalFenceGPU(_fence, _current_epoch_cpu, phi::queue_type::direct); // GPU starts catching up to current epoch
    ++_current_epoch_cpu;                                                          // new epoch begins
}

pr::gpu_epoch_t pr::gpu_epoch_tracker::get_current_epoch_gpu() const { return _backend->getFenceValue(_fence); }
