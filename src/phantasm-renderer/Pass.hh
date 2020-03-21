#pragma once

#include <rich-log/log.hh>

#include <phantasm-renderer/PrimitivePipeline.hh>
#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/fwd.hh>

namespace pr
{
class Pass
{
public:
    Pass(Frame* parent) : mParent(parent) {}

    Pass(Pass const&) = delete;
    Pass(Pass&&) noexcept = delete;
    Pass& operator=(Pass const&) = delete;
    Pass& operator=(Pass&&) noexcept = delete; // TODO: allow move

    ~Pass();

public:
    // lvalue-qualified as pass has to stay alive
    PrimitivePipeline pipeline(graphics_pipeline_state const& graphics_pipeline) & { return {mParent, graphics_pipeline.data._handle}; }

    // TODO: cache-access version

private:
    Frame* mParent;
};
}
