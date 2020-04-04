#include "Framebuffer.hh"

#include "Frame.hh"

pr::Framebuffer::~Framebuffer() { destroy(); }

pr::GraphicsPass pr::Framebuffer::make_pass(const pr::graphics_pass_info& gp) &
{
    return {mParent, mParent->framebufferAcquireGraphicsPSO(gp, mHashInfo)};
}

void pr::Framebuffer::destroy()
{
    if (mParent)
        mParent->framebufferOnJoin(*this);
}

pr::Framebuffer pr::framebuffer_builder::make() { return _parent->buildFramebuffer(_cmd); }
