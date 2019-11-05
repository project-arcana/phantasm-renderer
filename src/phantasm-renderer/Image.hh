#pragma once

#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fwd.hh>

#include <typed-geometry/tg-lean.hh>

namespace pr
{
/**
 * a move-only type representing an n-dimensional image
 */
template <int D, class T, bool IsFrameLocal>
class Image
{
    // getter
public:
    tg::size<D, int> size() const { return mSize; }
    pr::format format() const { return mFormat; }

    // move-only
private:
    Image(Image const&) = delete;
    Image(Image&&) = default;
    Image& operator=(Image const&) = delete;
    Image& operator=(Image&&) = default;

private:
    tg::size<D, int> mSize;
    pr::format mFormat;
};
}
