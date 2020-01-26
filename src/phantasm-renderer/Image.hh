#pragma once

#include <phantasm-renderer/common/resource_info.hh>
#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fwd.hh>

#include <typed-geometry/tg-lean.hh>

namespace pr
{
/**
 * a move-only type representing an n-dimensional image
 */
template <int D, format F>
class Image
{
    // getter
public:
    tg::size<D, int> size() const { return mSize; }
    int arraySize() const { return mArrayLevels; }
    pr::format format() const { return F; }

    // ctor
public:
    Image(phi::handle::resource res, texture_info const& tex_info) : mResource(res), mIsRenderTarget(false)
    {
        mInfo.tex_info = tex_info;
        mSize.width = tex_info.width;
        if constexpr (D > 1)
        {
            mSize.height = tex_info.height;
        }
        if constexpr (D > 2)
        {
            mSize.depth = tex_info.depth_or_array_size;
        }
        else
        {
            mArrayLevels = tex_info.depth_or_array_size;
        }
    }

    Image(phi::handle::resource res, render_target_info const& rt_info) : mResource(res), mIsRenderTarget(true)
    {
        static_assert(D == 2, "render targets must be 2D");

        mInfo.rt_info = rt_info;
        mSize = {rt_info.width, rt_info.height};
        mArrayLevels = 1;
    }

    // internal
public:
    phi::handle::resource getHandle() const { return mResource; }
    bool isRenderTarget() const { return mIsRenderTarget; }
    texture_info const& getTexInfo() const { return mInfo.tex_info; }
    render_target_info const& getRTInfo() const { return mInfo.rt_info; }

    // move-only
private:
    Image(Image const&) = delete;
    Image(Image&&) = default;
    Image& operator=(Image const&) = delete;
    Image& operator=(Image&&) = default;

private:
    phi::handle::resource mResource; ///< PHI resource
    tg::size<D, int> mSize;          ///< size of this texture
    int mArrayLevels = 1;            ///< size of the array, usually 1

    bool mIsRenderTarget = false;
    union {
        texture_info tex_info;
        render_target_info rt_info;
    } mInfo;
};
}
