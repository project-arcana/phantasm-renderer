#pragma once

#include <phantasm-renderer/fwd.hh>

#include "types.hh"

namespace pr::backend
{
class BackendInterface
{
    // reference type
public:
    BackendInterface(BackendInterface const&) = delete;
    BackendInterface(BackendInterface&&) = delete;
    BackendInterface& operator=(BackendInterface const&) = delete;
    BackendInterface& operator=(BackendInterface&&) = delete;
    virtual ~BackendInterface() = default;

protected:
    BackendInterface() = default;

private:
};
}
