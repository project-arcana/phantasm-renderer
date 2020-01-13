#pragma once

#include <cstddef>

namespace pr::backend::mem
{
template <class T>
constexpr T kb(T x)
{
    return x * 1024;
}

template <class T>
constexpr T mb(T x)
{
    return x * 1024 * 1024;
}

// From D3D12 sample MiniEngine
// https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Math/Common.h
template <class T>
[[nodiscard]] T align_up_masked(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <class T>
[[nodiscard]] T align_down_masked(T value, size_t mask)
{
    return (T)((size_t)value & ~mask);
}

template <class T>
[[nodiscard]] T align_up(T value, size_t alignment)
{
    return align_up_masked(value, alignment - 1);
}

template <class T>
[[nodiscard]] T align_down(T value, size_t alignment)
{
    return align_down_masked(value, alignment - 1);
}

template <class T>
[[nodiscard]] bool is_aligned(T value, size_t alignment)
{
    return 0 == ((size_t)value & (alignment - 1));
}

template <class T>
[[nodiscard]] T divide_by_multiple(T value, size_t alignment)
{
    return (T)((value + alignment - 1) / alignment);
}

template <class T>
[[nodiscard]] T align_offset(T offset, size_t alignment)
{
    return ((offset + (alignment - 1)) & ~(alignment - 1));
}

template <class T>
[[nodiscard]] bool is_power_of_two(T value)
{
    return value != 0 && (value & (value - 1)) == 0;
}
}
