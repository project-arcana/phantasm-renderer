#pragma once
#ifdef PR_BACKEND_D3D12

#include <dxgiformat.h> // This header only contains a single enum, no includes believe it or not

#include <typed-geometry/detail/comp_traits.hh>

namespace pr::backend::d3d12::util
{
template <class T>
[[nodiscard]] constexpr DXGI_FORMAT get_dxgi_format()
{
    if constexpr (tg::is_comp_like<T, 4, float>)
        return DXGI_FORMAT_R32G32B32_FLOAT;
    else if constexpr (tg::is_comp_like<T, 3, float>)
        return DXGI_FORMAT_R32G32B32_FLOAT;
    else if constexpr (tg::is_comp_like<T, 2, float>)
        return DXGI_FORMAT_R32G32_FLOAT;
    else if constexpr (tg::is_comp_like<T, 1, float>)
        return DXGI_FORMAT_R32_FLOAT;

    else if constexpr (tg::is_comp_like<T, 4, int>)
        return DXGI_FORMAT_R32G32B32_SINT;
    else if constexpr (tg::is_comp_like<T, 3, int>)
        return DXGI_FORMAT_R32G32B32_SINT;
    else if constexpr (tg::is_comp_like<T, 2, int>)
        return DXGI_FORMAT_R32G32_SINT;
    else if constexpr (tg::is_comp_like<T, 1, int>)
        return DXGI_FORMAT_R32_SINT;

    else if constexpr (tg::is_comp_like<T, 4, unsigned>)
        return DXGI_FORMAT_R32G32B32_UINT;
    else if constexpr (tg::is_comp_like<T, 3, unsigned>)
        return DXGI_FORMAT_R32G32B32_UINT;
    else if constexpr (tg::is_comp_like<T, 2, unsigned>)
        return DXGI_FORMAT_R32G32_UINT;
    else if constexpr (tg::is_comp_like<T, 1, unsigned>)
        return DXGI_FORMAT_R32_UINT;

    else
    {
        static_assert(sizeof(T) == 0, "Unknown type");
        return DXGI_FORMAT_UNKNOWN;
    }
}

template <class T>
static constexpr DXGI_FORMAT dxgi_format = get_dxgi_format<T>();

}

#endif
