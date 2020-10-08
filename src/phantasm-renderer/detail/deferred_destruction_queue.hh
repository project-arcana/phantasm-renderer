#pragma once

#include <clean-core/alloc_vector.hh>

#include <phantasm-hardware-interface/handles.hh>

#include <phantasm-renderer/fwd.hh>

namespace pr
{
/// persistent queue of (PHI) resources pending destruction
/// keeps track of GPU epochs and automatically frees older resources when enqueueing
struct deferred_destruction_queue
{
    void free(pr::Context& ctx, phi::handle::shader_view sv);
    void free(pr::Context& ctx, phi::handle::resource res);
    void free_range(pr::Context& ctx, cc::span<phi::handle::resource const> res_range);

    unsigned free_all_pending(pr::Context& ctx);

    void initialize(cc::allocator* alloc, unsigned num_reserved_svs = 128, unsigned num_reserved_res = 128);
    void destroy(pr::Context& ctx);

private:
    gpu_epoch_t gpu_epoch_old = 0;
    gpu_epoch_t gpu_epoch_new = 0;

    cc::alloc_vector<phi::handle::shader_view> pending_svs_old;
    cc::alloc_vector<phi::handle::shader_view> pending_svs_new;
    cc::alloc_vector<phi::handle::resource> pending_res_old;
    cc::alloc_vector<phi::handle::resource> pending_res_new;
};
}
