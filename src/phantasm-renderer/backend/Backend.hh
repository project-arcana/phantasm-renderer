#pragma once

#include <phantasm-renderer/fwd.hh>

namespace pr
{
class Backend
{
    // reference type
public:
    Backend(Backend const&) = delete;
    Backend(Backend&&) = delete;
    Backend& operator=(Backend const&) = delete;
    Backend& operator=(Backend&&) = delete;
    virtual ~Backend() = default;

protected:
    Backend() = default;

private:
};
}
