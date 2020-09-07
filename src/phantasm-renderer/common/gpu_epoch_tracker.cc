#include "gpu_epoch_tracker.hh"

#include <clean-core/assert.hh>
#include <clean-core/span.hh>

#include <phantasm-hardware-interface/Backend.hh>

void pr::gpu_epoch_tracker::initialize(phi::Backend* backend)
{
    _fence = backend->createFence();
    CC_ASSERT(backend->getFenceValue(_fence) == 0 && "invalid fence value on init");
}

void pr::gpu_epoch_tracker::destroy(phi::Backend* backend) { backend->free(cc::span{_fence}); }

pr::gpu_epoch_t pr::gpu_epoch_tracker::get_current_epoch_gpu(phi::Backend* backend) const { return backend->getFenceValue(_fence); }
