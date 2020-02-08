#pragma once

#include <rich-log/log.hh>

#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/PrimitivePipeline.hh>

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
    PrimitivePipeline pipeline()
    {

    }

private:
    Frame* mParent;
};
}
