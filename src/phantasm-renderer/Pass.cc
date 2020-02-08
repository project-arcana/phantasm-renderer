#include "Pass.hh"

#include "Frame.hh"

pr::Pass::~Pass()
{
    mParent->passOnJoin(*this);
}
