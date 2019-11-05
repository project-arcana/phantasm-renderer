#pragma once

#include <phantasm-renderer/Frame.hh>

namespace pr
{
/**
 * A CompiledFrame is a fully baked command buffer ready for submission
 */
class CompiledFrame
{
    // move-only type
public:
    CompiledFrame(CompiledFrame const&) = delete;
    CompiledFrame(CompiledFrame&&) = default;
    CompiledFrame& operator=(CompiledFrame const&) = delete;
    CompiledFrame& operator=(CompiledFrame&&) = default;

private:
    CompiledFrame() = default;
    friend CompiledFrame compile(Frame const&);

    // member
private:
    // TODO
};
}
