#pragma once

#include <clean-core/vector.hh>

#include <phantasm-hardware-interface/types.hh>

#include <phantasm-renderer/common/state_info.hh>

namespace pr
{
/**
 * A CompiledFrame is a compiled command list ready for submission
 */
class CompiledFrame
{
    // move-only type
public:
    CompiledFrame() = default;
    CompiledFrame(CompiledFrame const&) = delete;
    CompiledFrame& operator=(CompiledFrame const&) = delete;
    CompiledFrame(CompiledFrame&& rhs) noexcept : parent(rhs.parent), event(rhs.event), cmdlist(rhs.cmdlist), freeables(cc::move(rhs.freeables))
    {
        rhs.parent = nullptr;
    }

    CompiledFrame& operator=(CompiledFrame&& rhs) noexcept
    {
        if (this != &rhs)
        {
            _destroy();
            parent = rhs.parent;
            event = rhs.event;
            cmdlist = rhs.cmdlist;
            freeables = cc::move(rhs.freeables);
            rhs.parent = nullptr;
        }

        return *this;
    }

    ~CompiledFrame() { _destroy(); }

private:
    friend class Context;
    CompiledFrame(phi::handle::command_list cmdlist, phi::handle::event event, cc::vector<freeable_cached_obj>&& freeables)
      : event(event), cmdlist(cmdlist), freeables(cc::move(freeables))
    {
    }

    void _destroy();

    Context* parent = nullptr;
    phi::handle::event event = phi::handle::null_event;
    phi::handle::command_list cmdlist = phi::handle::null_command_list;
    cc::vector<freeable_cached_obj> freeables;
};
}
