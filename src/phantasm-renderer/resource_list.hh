#pragma once

#include <clean-core/always_false.hh>

#include <type_traits>

#include <phantasm-renderer/fwd.hh>

/**
 * Template metaprogramming helpers for lists of resources
 *
 * - resource_list<...>
 * - empty_resource_list
 * - is_empty_resource_list<L>
 * - concat_resource_list<A, B>
 * - append_resource_list<A, ...>
 * - remove_resource_prefix<A, ...>
 */

namespace pr
{
template <class... Resources>
class resource_list;

using empty_resource_list = resource_list<>;

template <class T>
static constexpr bool is_empty_resource_list = std::is_same_v<T, empty_resource_list>;

namespace detail
{
template <class A, class B>
struct concat_resource_list_t;
template <class... A, class... B>
struct concat_resource_list_t<resource_list<A...>, resource_list<B...>>
{
    using type = resource_list<A..., B...>;
};

template <class A, class... B>
struct append_resource_list_t;
template <class... A, class... B>
struct append_resource_list_t<resource_list<A...>, B...>
{
    using type = resource_list<A..., B...>;
};

template <class A, class... B>
struct remove_resource_prefix_t
{
    static_assert(cc::always_false<A>, "resource list not compatible");
};
template <class A>
struct remove_resource_prefix_t<A>
{
    using type = A;
};
template <class Match, class... R1, class... R2>
struct remove_resource_prefix_t<resource_list<Match, R1...>, Match, R2...>
{
    using type = typename remove_resource_prefix_t<resource_list<R1...>, R2...>::type;
};

template <class VertexT, class FragmentT, class BoundResourceList, class UnboundResourceList>
struct bound_pipeline_type_t;
template <class VertexT, class FragmentT, class BoundResourceList, class... UnboundResources>
struct bound_pipeline_type_t<VertexT, FragmentT, BoundResourceList, resource_list<UnboundResources...>>
{
    using type = PrimitivePipeline<VertexT, FragmentT, BoundResourceList, UnboundResources...>;
};
}

template <class A, class B>
using concat_resource_list = typename detail::concat_resource_list_t<A, B>::type;

template <class A, class... B>
using append_resource_list = typename detail::append_resource_list_t<A, B...>::type;

template <class A, class... B>
using remove_resource_prefix = typename detail::remove_resource_prefix_t<A, B...>::type;

template <class VertexT, class FragmentT, class BoundResourceList, class UnboundResourceList>
using bound_pipeline_type = typename detail::bound_pipeline_type_t<VertexT, FragmentT, BoundResourceList, UnboundResourceList>::type;
}
