#include "Framebuffer.hh"

#include "Frame.hh"

pr::raii::Framebuffer::~Framebuffer() { destroy(); }

pr::raii::GraphicsPass pr::raii::Framebuffer::make_pass(const pr::graphics_pass_info& gp) &
{
    return {mParent, mParent->framebufferAcquireGraphicsPSO(gp, mHashInfo)};
}

void pr::raii::Framebuffer::destroy()
{
    if (mParent)
        mParent->framebufferOnJoin(*this);
}

pr::raii::Framebuffer pr::raii::framebuffer_builder::make() { return _parent->buildFramebuffer(_cmd); }
