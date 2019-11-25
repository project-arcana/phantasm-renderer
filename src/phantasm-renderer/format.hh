#pragma once

namespace pr
{
/// Internal image formats supported by GPUs
enum class format
{
    r8,
    r16f,
    r32f,

    rg8,
    rg16f,
    rg32f,

    rgb8,
    rgb16f,
    rgb32f,

    rgba8,
    rgba16f,
    rgba32f,

    // srgb formats
    srgb8,
    srgb8_alpha8,

    depth32f,

    // TODO!

    count
};
}
