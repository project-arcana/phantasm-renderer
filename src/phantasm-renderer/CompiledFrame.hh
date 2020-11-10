#pragma once

#include <clean-core/alloc_vector.hh>

#include <phantasm-hardware-interface/handles.hh>

#include <phantasm-renderer/common/state_info.hh>

namespace pr
{
class Context;

/// A compiled command list ready for submission, as received from Context::compile(frame)
/// move-only, must be submitted or discarded via the Context before destruction
class CompiledFrame
{
public:
    CompiledFrame() = default;

    /// whether this is properly constructed (and not moved-from)
    bool is_valid() const { return _valid; }

    CompiledFrame(CompiledFrame const&) = delete;
    CompiledFrame& operator=(CompiledFrame const&) = delete;

    CompiledFrame(CompiledFrame&& rhs) noexcept
      : _valid(rhs._valid),
        _cmdlist(rhs._cmdlist),
        _freeables(cc::move(rhs._freeables)),
        _deferred_free_resources(cc::move(rhs._deferred_free_resources)),
        _present_after_submit_swapchain(rhs._present_after_submit_swapchain)
    {
        rhs.invalidate();
    }

    CompiledFrame& operator=(CompiledFrame&& rhs) noexcept
    {
        if (this != &rhs)
        {
            // if this is already valid, this move would delete a submission-ready command list
            CC_ASSERT(!is_valid() && "all compiled frames must be submitted or discarded via the Context");

            _valid = rhs._valid;
            _cmdlist = rhs._cmdlist;
            _freeables = cc::move(rhs._freeables);
            _deferred_free_resources = cc::move(rhs._deferred_free_resources);
            _present_after_submit_swapchain = rhs._present_after_submit_swapchain;

            rhs.invalidate();
        }

        return *this;
    }

    ~CompiledFrame() { CC_ASSERT(!is_valid() && "all compiled frames must be submitted or discarded via the Context"); }

private:
    friend class Context;
    CompiledFrame(phi::handle::command_list cmdlist,
                  cc::alloc_vector<freeable_cached_obj>&& freeables,
                  cc::alloc_vector<phi::handle::resource>&& deferred_free_resources,
                  phi::handle::swapchain present_after_submit_sc)
      : _valid(true),
        _cmdlist(cmdlist),
        _freeables(cc::move(freeables)),
        _deferred_free_resources(cc::move(deferred_free_resources)),
        _present_after_submit_swapchain(present_after_submit_sc)
    {
    }

    void invalidate() { _valid = false; }

    bool _valid = false;
    phi::handle::command_list _cmdlist = phi::handle::null_command_list;
    cc::alloc_vector<freeable_cached_obj> _freeables;
    cc::alloc_vector<phi::handle::resource> _deferred_free_resources;
    phi::handle::swapchain _present_after_submit_swapchain = phi::handle::null_swapchain;
};
}
