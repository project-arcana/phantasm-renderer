#pragma once

#include <phantasm-renderer/Buffer.hh>
#include <phantasm-renderer/FragmentShader.hh>
#include <phantasm-renderer/Image.hh>
#include <phantasm-renderer/Pass.hh>
#include <phantasm-renderer/VertexShader.hh>
#include <phantasm-renderer/default_config.hh>
#include <phantasm-renderer/format.hh>
#include <phantasm-renderer/fragment_type.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/immediate.hh>
#include <phantasm-renderer/resources.hh>
#include <phantasm-renderer/vertex_type.hh>

#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg-lean.hh>

namespace pr
{
CompiledFrame compile(Frame const& frame);

class Frame
{
    // creation API
public:
    template <class T>
    Image<1, T, true> make_image(int width, T initialValue)
    {
        return {}; // TODO
    }
    template <class T>
    Image<2, T, true> make_image(tg::isize2 size, T initialValue)
    {
        return {}; // TODO
    }
    template <class T>
    Image<3, T, true> make_image(tg::isize3 size, T initialValue)
    {
        return {}; // TODO
    }

    template <class T>
    Buffer<T[]> make_buffer(cc::span<T> data)
    {
        return {}; // TODO
    }
    template <class T, class = std::enable_if_t<std::is_trivially_copyable_v<T>>>
    Buffer<T> make_buffer(T const& data)
    {
        return {}; // TODO
    }
    template <class T>
    Buffer<T> make_uninitialized_buffer(size_t size);

    template <class... FragmentT>
    FragmentShader<fragment_type_of<FragmentT...>> make_fragment_shader(cc::string_view code)
    {
        return {}; // TODO
    }
    template <class FragmentT> // must be set to void!
    FragmentShader<void> make_fragment_shader()
    {
        static_assert(std::is_same_v<FragmentT, void>, "empty fragment shader must be void");
        return {}; // TODO
    }
    template <class... VertexT>
    VertexShader<vertex_type_of<VertexT...>> make_vertex_shader(cc::string_view code)
    {
        return {}; // TODO
    }

    // pass RAII API
public:
    template <int D, class FragmentT, class DepthT, bool IsLocalF, bool IsLocalD>
    Pass<FragmentT, empty_resource_list> render_to(Image<D, FragmentT, IsLocalF> const& img, Image<D, DepthT, IsLocalD> const& depth_buffer)
    {
        return {}; // TODO
    }
    template <int D, class FragmentT, class DepthT, bool IsLocalF, bool IsLocalD>
    Pass<FragmentT, empty_resource_list> render_to(Image<D, FragmentT, IsLocalF> const& img, Image<D, DepthT, IsLocalD> const& depth_buffer, tg::iaabb2 viewport)
    {
        return {}; // TODO
    }

    // move-only type
public:
    Frame(Frame const&) = delete;
    Frame(Frame&&) = default;
    Frame& operator=(Frame const&) = delete;
    Frame& operator=(Frame&&) = default;

private:
    Frame() = default;
    friend class Context;

    // members
private:
    // TODO: Context weak_ptr
};
}
