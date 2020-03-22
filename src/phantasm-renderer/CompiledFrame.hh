#pragma once

#include <phantasm-hardware-interface/types.hh>

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
    CompiledFrame(CompiledFrame&&) noexcept = default;
    CompiledFrame& operator=(CompiledFrame&&) noexcept = default;

private:
    friend class Context;
    CompiledFrame(phi::handle::command_list cmdlist, phi::handle::event event) : event(event), cmdlist(cmdlist) {}
    phi::handle::event event = phi::handle::null_event;
    phi::handle::command_list cmdlist = phi::handle::null_command_list;
};
}
