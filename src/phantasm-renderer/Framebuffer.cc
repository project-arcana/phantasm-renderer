#include "Framebuffer.hh"

#include "Frame.hh"

pr::Framebuffer::~Framebuffer() { destroy(); }

void pr::Framebuffer::destroy()
{
    if (mParent)
        mParent->framebufferOnJoin(*this);
}

pr::Framebuffer pr::framebuffer_builder::make() { return _parent->buildFramebuffer(_cmd); }
