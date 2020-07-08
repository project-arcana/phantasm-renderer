#include "Framebuffer.hh"

#include "Frame.hh"

pr::raii::Framebuffer::~Framebuffer() { destroy(); }

pr::raii::GraphicsPass pr::raii::Framebuffer::make_pass(const pr::graphics_pass_info& gp) &
{
    return {mParent, mParent->framebufferAcquireGraphicsPSO(gp, mHashInfo, mNumSamples)};
}

void pr::raii::Framebuffer::sort_drawcalls_by_pso(unsigned num_drawcalls)
{
    mParent->framebufferOnSortByPSO(num_drawcalls);
}

void pr::raii::Framebuffer::destroy()
{
    if (mParent)
        mParent->framebufferOnJoin(*this);
}

pr::raii::Framebuffer pr::raii::framebuffer_builder::make()
{
    return _parent->buildFramebuffer(_cmd, _num_samples, _has_custom_blendstate ? &_blendstate_overrides : nullptr);
}
