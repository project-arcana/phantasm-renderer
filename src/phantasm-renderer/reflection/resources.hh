#pragma once

#include <type_traits>

#include <reflector/introspect.hh>

#include <phantasm-hardware-interface/arguments.hh>

namespace pr
{
namespace detail
{
struct arg_shape_visitor
{
    phi::arg::shader_arg_shape shape;

    template <class T>
    void operator()(T const& ref, char const* name)
    {
        // TODO: do checks on T, decide of SRV, UAV, Sampler or part of merged CBV
    }
};

}

template <class T>
[[nodiscard]] constexpr phi::arg::shader_arg_shape reflect_arg_shape()
{
    static_assert(rf::is_introspectable<T>, "resource type not introspectable");

    detail::arg_shape_visitor visitor;
    T* volatile dummy_ptr = nullptr;
    rf::do_introspect(visitor, *dummy_ptr);
    return visitor.shape;
}

}
