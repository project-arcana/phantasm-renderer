#pragma once

namespace pr
{
template <class... ComponentsT>
struct fragment_type;

namespace detail
{
template <class... ComponentsT>
struct fragment_type_of
{
    using type = fragment_type<ComponentsT...>;
};
template <class T>
struct fragment_type_of<T>
{
    using type = T;
};
}

template <class... ComponentsT>
using fragment_type_of = typename detail::fragment_type_of<ComponentsT...>::type;
}
