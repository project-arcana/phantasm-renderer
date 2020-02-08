#pragma once

#include "format.hh"
#include "view_type.hh"

namespace phi
{
struct backend_config;
struct window_handle;
class Backend;
}

namespace pr
{
class Device;
class Context;
class Queue;

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

class Frame;
class CompiledFrame;

class Pass;
class PrimitivePipeline;

struct primitive_pipeline_config;
}
