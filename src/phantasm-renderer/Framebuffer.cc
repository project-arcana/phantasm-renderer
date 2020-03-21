#include "Framebuffer.hh"

#include "Frame.hh"

pr::Framebuffer::~Framebuffer() { mParent->framebufferOnJoin(*this); }
