#include "deferred_destruction_queue.hh"

#include <clean-core/utility.hh>

#include <phantasm-hardware-interface/Backend.hh>

#include <phantasm-renderer/Context.hh>
#include <phantasm-renderer/common/log.hh>

void pr::deferred_destruction_queue::free(pr::Context& ctx, phi::handle::shader_view sv)
{
    auto lg = std::lock_guard(mutex);
    _free_pending_unsynced(ctx);
    pending_svs_new.push_back(sv);
}

void pr::deferred_destruction_queue::free(pr::Context& ctx, phi::handle::resource res)
{
    auto lg = std::lock_guard(mutex);
    _free_pending_unsynced(ctx);
    pending_res_new.push_back(res);
}

void pr::deferred_destruction_queue::free(pr::Context& ctx, phi::handle::pipeline_state pso)
{
    auto lg = std::lock_guard(mutex);
    _free_pending_unsynced(ctx);
    pending_psos_new.push_back(pso);
}

void pr::deferred_destruction_queue::free_range(pr::Context& ctx, cc::span<const phi::handle::resource> res_range)
{
    auto lg = std::lock_guard(mutex);
    _free_pending_unsynced(ctx);
    if (res_range.size() > 0)
    {
        pending_res_new.push_back_range(res_range);
    }
}

void pr::deferred_destruction_queue::free_range(pr::Context& ctx, cc::span<const phi::handle::shader_view> sv_range)
{
    auto lg = std::lock_guard(mutex);
    _free_pending_unsynced(ctx);
    if (sv_range.size() > 0)
    {
        pending_svs_new.push_back_range(sv_range);
    }
}

void pr::deferred_destruction_queue::free_to_cache(pr::Context& ctx, phi::handle::resource res)
{
    auto lg = std::lock_guard(mutex);
    _free_pending_unsynced(ctx);
    pending_cached_res_new.push_back(res);
}

void pr::deferred_destruction_queue::free_range_to_cache(pr::Context& ctx, cc::span<phi::handle::resource const> res_range)
{
    auto lg = std::lock_guard(mutex);
    _free_pending_unsynced(ctx);
    if (res_range.size() > 0)
    {
        pending_cached_res_new.push_back_range(res_range);
    }
}

unsigned pr::deferred_destruction_queue::free_all_pending(pr::Context& ctx)
{
    auto lg = std::lock_guard(mutex);
    return _free_pending_unsynced(ctx);
}

void pr::deferred_destruction_queue::initialize(cc::allocator* alloc, unsigned num_reserved_svs, unsigned num_reserved_res, unsigned num_reserved_psos)
{
    pending_svs_old.reset_reserve(alloc, num_reserved_svs);
    pending_svs_new.reset_reserve(alloc, num_reserved_svs);
    pending_psos_old.reset_reserve(alloc, num_reserved_psos);
    pending_psos_old.reset_reserve(alloc, num_reserved_psos);
    pending_res_old.reset_reserve(alloc, num_reserved_res);
    pending_res_new.reset_reserve(alloc, num_reserved_res);
    pending_cached_res_old.reset_reserve(alloc, num_reserved_res);
    pending_cached_res_new.reset_reserve(alloc, num_reserved_res);
}

void pr::deferred_destruction_queue::destroy(pr::Context& ctx)
{
    auto lg = std::lock_guard(mutex);
    ctx.get_backend().freeRange(pending_svs_old);
    ctx.get_backend().freeRange(pending_svs_new);
    ctx.get_backend().freeRange(pending_res_old);
    ctx.get_backend().freeRange(pending_res_new);
    ctx.get_backend().freeRange(pending_cached_res_old);
    ctx.get_backend().freeRange(pending_cached_res_new);
    for (auto pso : pending_psos_old)
    {
        ctx.get_backend().free(pso);
    }
    for (auto pso : pending_psos_new)
    {
        ctx.get_backend().free(pso);
    }
    pending_svs_old = {};
    pending_svs_new = {};
    pending_psos_old = {};
    pending_psos_old = {};
    pending_res_old = {};
    pending_res_new = {};
    pending_cached_res_old = {};
    pending_cached_res_new = {};
}

unsigned pr::deferred_destruction_queue::_free_pending_unsynced(pr::Context& ctx)
{
    auto const epoch_cpu = ctx.get_current_cpu_epoch();
    auto const epoch_gpu = ctx.get_current_gpu_epoch();

    //    PR_LOG("cull: CPU: {}, GPU: {}", epoch_cpu, epoch_gpu);
    //    PR_LOG("    old: {}, new: {}", gpu_epoch_old, gpu_epoch_new);

    unsigned num_freed = 0;
    // NOTE: on vulkan there is some sort of issue with out-of-order submission
    // which requires this safety buffer - or it's an overzelaous validation layer
    if (epoch_gpu >= gpu_epoch_old + 2)
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
        for (auto cres : pending_cached_res_old)
        {
            ++num_freed;
            ctx.free_to_cache_untyped({cres});
        }
        for (auto pso : pending_psos_old)
        {
            ++num_freed;
            ctx.get_backend().free(pso);
        }

        cc::swap(pending_svs_old, pending_svs_new);
        cc::swap(pending_res_old, pending_res_new);
        cc::swap(pending_cached_res_old, pending_cached_res_new);
        cc::swap(pending_psos_old, pending_psos_new);
        pending_svs_new.clear();
        pending_res_new.clear();
        pending_psos_new.clear();
        pending_cached_res_new.clear();

        gpu_epoch_old = gpu_epoch_new;
        // PR_LOG("freed {} elements as GPU progressed enough", num_freed);
    }

    CC_ASSERT(epoch_cpu >= gpu_epoch_new && ">400 year overflow or programmer error");
    gpu_epoch_new = epoch_cpu;

    return num_freed;
}
