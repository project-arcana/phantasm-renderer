#pragma once

namespace pr
{
template <class... ComponentsT>
struct vertex_type;

namespace detail
{
template <class... ComponentsT>
struct vertex_type_of
{
    using type = vertex_type<ComponentsT...>;
};
template <class T>
struct vertex_type_of<T>
{
    using type = T;
};
}

template <class... ComponentsT>
using vertex_type_of = typename detail::vertex_type_of<ComponentsT...>::type;

}
