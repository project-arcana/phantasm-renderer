#include "resource_types.hh"

#include "Context.hh"

void pr::resource::_destroy()
{
    if (_parent != nullptr)
    {
        _parent->freeResource(_handle);
    }
}

pr::cached_buffer::~cached_buffer()
{
    auto* const parent = _internal._resource._parent;
    if (parent != nullptr)
    {
        parent->freeCachedBuffer(_internal._info, cc::move(_internal._resource));
    }
}

pr::cached_render_target::~cached_render_target()
{
    auto* const parent = _internal._resource._parent;
    if (parent != nullptr)
    {
        parent->freeCachedTarget(_internal._info, cc::move(_internal._resource));
    }
}

void pr::shader_binary::_destroy()
{
    if (_parent != nullptr && _owning_blob != nullptr)
    {
        _parent->freeShaderBinary(_owning_blob);
    }
}
