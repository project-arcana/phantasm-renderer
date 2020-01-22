#pragma once

#include <clean-core/poly_unique_ptr.hh>
#include <clean-core/span.hh>
#include <clean-core/string_view.hh>

#include <typed-geometry/tg-lean.hh>

#include <phantasm-renderer/Buffer.hh>
#include <phantasm-renderer/FragmentShader.hh>
#include <phantasm-renderer/Image.hh>
#include <phantasm-renderer/PrimitivePipeline.hh>
#include <phantasm-renderer/VertexShader.hh>
#include <phantasm-renderer/fragment_type.hh>
#include <phantasm-renderer/fwd.hh>
#include <phantasm-renderer/vertex_type.hh>

#include <phantasm-renderer/backend/types.hh>
#include <phantasm-renderer/backend/window_handle.hh>

namespace pr
{
/**
 * The context is the central manager of all graphics resources including the backend
 *
 *
 */
class Context
{
    // creation API
public:
    Frame make_frame();

    template <format F>
    Image1D<F> make_image(int width);
    template <format F>
    Image2D<F> make_image(tg::isize2 size);
    template <format F>
    Image3D<F> make_image(tg::isize3 size);

    template <class T>
    Buffer<T[]> make_buffer(cc::span<T> data);
    template <class T, class = std::enable_if_t<std::is_trivially_copyable_v<T>>>
    Buffer<T> make_buffer(T const& data);
    template <class T>
    Buffer<T> make_uninitialized_buffer(size_t size);

    template <format FragmentF>
    FragmentShader<FragmentF> make_fragment_shader(cc::string_view code);
    //    template <class FragmentT> // must be set to void!
    //    FragmentShader<void> make_fragment_shader()
    //    {
    //        static_assert(std::is_same_v<FragmentT, void>, "empty fragment shader must be void");
    //        return {}; // TODO
    //    }
    template <class... VertexT>
    VertexShader<vertex_type_of<VertexT...>> make_vertex_shader(cc::string_view code)
    {
        return {}; // TODO
    }

    // consumption API
public:
    void submit(Frame const& frame);
    void submit(CompiledFrame const& frame);

    // ctors
public:
    /// constructs a context with a default backend (usually vulkan)
    Context(backend::window_handle const& window_handle);
    /// constructs a context with a specified backend
    Context(cc::poly_unique_ptr<backend::Backend> backend);

    ~Context();

    // ref type
public:
    Context(Context const&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context const&) = delete;
    Context& operator=(Context&&) = delete;

    // members
private:
    cc::poly_unique_ptr<backend::Backend> mBackend;
};

// ============================== IMPLEMENTATION ==============================

// TODO
}
