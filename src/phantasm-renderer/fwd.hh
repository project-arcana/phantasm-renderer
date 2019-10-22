#pragma once

#include "image_view_type.hh"

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

template <int D, class T>
class Image;
template <class T>
using Image1D = Image<1, T>;
template <class T>
using Image2D = Image<2, T>;
template <class T>
using Image3D = Image<3, T>;

// TODO: or just view?
template <image_view_type Type, class T>
class ImageView;
template <class T>
using ImageView1D = ImageView<image_view_type::image_1D, T>;
template <class T>
using ImageView2D = ImageView<image_view_type::image_2D, T>;
template <class T>
using ImageView3D = ImageView<image_view_type::image_3D, T>;
template <class T>
using ImageView1DArray = ImageView<image_view_type::image_1D_array, T>;
template <class T>
using ImageView2DArray = ImageView<image_view_type::image_2D_array, T>;
template <class T>
using ImageViewCube = ImageView<image_view_type::image_cube, T>;
template <class T>
using ImageViewCubeArray = ImageView<image_view_type::image_cube_array, T>;

template <class T>
class Buffer;
template <class T>
class BufferView;
}
