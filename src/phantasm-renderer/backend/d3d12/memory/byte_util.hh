#pragma once

#include <cstddef>

namespace pr::backend::d3d12::mem
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

inline constexpr size_t a_64kb = kb(64);
inline constexpr size_t a_1mb = mb(1);
inline constexpr size_t a_2mb = mb(2);
inline constexpr size_t a_4mb = mb(4);
inline constexpr size_t a_8mb = mb(8);
inline constexpr size_t a_16mb = mb(16);
inline constexpr size_t a_32mb = mb(32);
inline constexpr size_t a_64mb = mb(64);
inline constexpr size_t a_128mb = mb(128);
inline constexpr size_t a_256mb = mb(256);

// From D3D12 sample MiniEngine
// https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Math/Common.h
template <typename T>
T align_up_masked(T value, size_t mask)
{
    return (T)(((size_t)value + mask) & ~mask);
}

template <typename T>
T align_down_masked(T value, size_t mask)
{
    return (T)((size_t)value & ~mask);
}

template <typename T>
T align_up(T value, size_t alignment)
{
    return align_up_masked(value, alignment - 1);
}

template <typename T>
T align_down(T value, size_t alignment)
{
    return align_down_masked(value, alignment - 1);
}

template <typename T>
bool is_aligned(T value, size_t alignment)
{
    return 0 == ((size_t)value & (alignment - 1));
}

template <typename T>
T divide_by_multiple(T value, size_t alignment)
{
    return (T)((value + alignment - 1) / alignment);
}

template <typename T>
T align_offset(T offset, size_t alignment)
{
    return ((offset + (alignment - 1)) & ~(alignment - 1));
}
}
