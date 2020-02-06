#pragma once

#include <phantasm-hardware-interface/types.hh>

namespace pr
{
/**
 * A CompiledFrame is a compiled command list ready for submission
 */
class CompiledFrame
{
    // ctor
public:
    CompiledFrame(phi::handle::command_list cmdlist, phi::handle::event event) : mRecordedEvent(event), mCmdlist(cmdlist) {}

    phi::handle::event getEvent() const { return mRecordedEvent; }
    phi::handle::command_list getCmdlist() const { return mCmdlist; }

    // move-only type
public:
    CompiledFrame(CompiledFrame const&) = delete;
    CompiledFrame(CompiledFrame&&) noexcept = default;
    CompiledFrame& operator=(CompiledFrame const&) = delete;
    CompiledFrame& operator=(CompiledFrame&&) noexcept = default;

    // member
private:
    phi::handle::event mRecordedEvent;
    phi::handle::command_list mCmdlist;
};
}
