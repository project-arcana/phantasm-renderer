#include "CompiledFrame.hh"

#include "Context.hh"

void pr::CompiledFrame::_destroy()
{
    if (parent != nullptr)
    {
        parent->discard(cc::move(*this));
    }
}
