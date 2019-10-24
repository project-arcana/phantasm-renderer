#pragma once

#include "view_type.hh"

namespace pr
{
class Device;
class Context;
class Queue;

class Frame;
class CompiledFrame;

template <class VertexT>
class VertexShader;
template <class FragmentT>
class FragmentShader;
class GeometryShader;
class TessellationControlShader;
class TessellationEvaluationShader;

template <int D, class T, bool IsFrameLocal = false>
class Image;
template <class T>
using Image1D = Image<1, T>;
template <class T>
using Image2D = Image<2, T>;
template <class T>
using Image3D = Image<3, T>;

template <class T>
class Buffer;

template <view_type Type, class T>
class View;
template <class T>
using ImageView1D = View<view_type::image_1D, T>;
template <class T>
using ImageView2D = View<view_type::image_2D, T>;
template <class T>
using ImageView3D = View<view_type::image_3D, T>;
template <class T>
using ImageView1DArray = View<view_type::image_1D_array, T>;
template <class T>
using ImageView2DArray = View<view_type::image_2D_array, T>;
template <class T>
using ImageViewCube = View<view_type::image_cube, T>;
template <class T>
using ImageViewCubeArray = View<view_type::image_cube_array, T>;
}
