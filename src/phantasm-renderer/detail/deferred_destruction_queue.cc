#include "deferred_destruction_queue.hh"

#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/Backend.hh>

#include <phantasm-renderer/Context.hh>

void pr::deferred_destruction_queue::free(pr::Context& ctx, phi::handle::shader_view sv)
{
    free_all_pending(ctx);
    pending_svs_new.push_back(sv);
}

void pr::deferred_destruction_queue::free(pr::Context& ctx, phi::handle::resource res)
{
    free_all_pending(ctx);
    pending_res_new.push_back(res);
}

void pr::deferred_destruction_queue::free_range(pr::Context& ctx, cc::span<const phi::handle::resource> res_range)
{
    free_all_pending(ctx);
    pending_res_new.push_back_span(res_range);
}

void pr::deferred_destruction_queue::initialize(cc::allocator* alloc, unsigned num_reserved_svs, unsigned num_reserved_res)
{
    pending_svs_old.reset_reserve(alloc, num_reserved_svs);
    pending_svs_new.reset_reserve(alloc, num_reserved_svs);
    pending_res_old.reset_reserve(alloc, num_reserved_res);
    pending_res_new.reset_reserve(alloc, num_reserved_res);
}

void pr::deferred_destruction_queue::destroy(pr::Context& ctx)
{
    ctx.get_backend().freeRange(pending_svs_old);
    ctx.get_backend().freeRange(pending_svs_new);
    ctx.get_backend().freeRange(pending_res_old);
    ctx.get_backend().freeRange(pending_res_new);
    pending_svs_old = {};
    pending_svs_new = {};
    pending_res_old = {};
    pending_res_new = {};
}

unsigned pr::deferred_destruction_queue::free_all_pending(pr::Context& ctx)
{
    auto const epoch_cpu = ctx.get_current_cpu_epoch();
    auto const epoch_gpu = ctx.get_current_gpu_epoch();

    unsigned num_freed = 0;
    if (epoch_gpu >= gpu_epoch_old)
    {
        // can free old
        if (pending_svs_old.size() > 0)
        {
            num_freed += unsigned(pending_svs_old.size());
            ctx.get_backend().freeRange(pending_svs_old);
        }
        if (pending_res_old.size() > 0)
        {
            num_freed += unsigned(pending_res_old.size());
            ctx.get_backend().freeRange(pending_res_old);
        }

        cc::swap(pending_svs_old, pending_svs_new);
        cc::swap(pending_res_old, pending_res_new);
        pending_svs_new.clear();
        pending_res_new.clear();

        gpu_epoch_old = gpu_epoch_new;
    }

    CC_ASSERT(epoch_cpu >= gpu_epoch_new && ">400 year overflow or programmer error");
    gpu_epoch_new = epoch_cpu;

    return num_freed;
}