#pragma once

#include <rich-log/log.hh>

#include <phantasm-renderer/GraphicsPass.hh>
#include <phantasm-renderer/common/growing_writer.hh>
#include <phantasm-renderer/fwd.hh>

namespace pr
{
class Framebuffer
{
public:
    Framebuffer(Frame* parent) : mParent(parent) {}

    Framebuffer(Framebuffer const&) = delete;
    Framebuffer(Framebuffer&&) noexcept = delete;
    Framebuffer& operator=(Framebuffer const&) = delete;
    Framebuffer& operator=(Framebuffer&&) noexcept = delete; // TODO: allow move

    ~Framebuffer();

public:
    // lvalue-qualified as pass has to stay alive
    GraphicsPass make_pass(graphics_pipeline_state const& graphics_pipeline) & { return {mParent, graphics_pipeline.data._handle}; }

    // TODO: cache-access version

private:
    Frame* mParent;
};
}
